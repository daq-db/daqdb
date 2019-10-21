/**
 *  Copyright (c) 2019 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "KVStore.h"

#include <boost/filesystem.hpp>
#include <cerrno>
#include <chrono>
#include <condition_variable>
#include <sstream>
#include <iostream>

#include <DhtServer.h>
#include <DhtUtils.h>
#include <Logger.h>
#include <daqdb/Types.h>
#include <libpmem.h>

using namespace std::chrono_literals;
namespace bf = boost::filesystem;

namespace DaqDB {

const size_t DEFAULT_KEY_SIZE = 16;

KVStoreBase *KVStore::Open(const DaqDB::Options &options) {
    KVStore *kvs = new KVStore(options);
    kvs->init();
    return kvs;
}

KVStore::KVStore(const DaqDB::Options &options)
    : _options(options), _spOffloadPoller(nullptr), _keySize(0) {}

KVStore::~KVStore() {
    DAQ_INFO("Closing DAQDB KVStore.");
    RTreeEngine::Close(_spRtree.get());
    _spRtree.reset();
    _spPKey.reset();
    _spDhtServer.reset();
    _spDht.reset();
    for (auto index = 0; index < _rqstPollers.size(); index++) {
        delete _rqstPollers.at(index);
    }
    _spOffloadPoller.reset();
}

void KVStore::init() {
    auto pollerCount = getOptions().runtime.numOfPollers;
    auto dhtCount = getOptions().dht.numOfDhtThreads;
    auto baseCoreId = getOptions().runtime.baseCoreId;
    auto coresUsed = 0;

    if (getOptions().runtime.logFunc)
        gLog.setLogFunc(getOptions().runtime.logFunc);

    DAQ_INFO("Starting DAQDB KVStore.");

    DAQ_INFO("Key structure:");
    for (size_t i = 0; i < getOptions().key.nfields(); i++) {
        DAQ_INFO("  Field[" + std::to_string(i) + "]: " +
                 std::to_string(getOptions().key.field(i).size) + " byte(s)" +
                 (getOptions().key.field(i).isPrimary ? " (primary)" : ""));
        _keySize += getOptions().key.field(i).size;
    }
    if (!_keySize)
        _keySize = DEFAULT_KEY_SIZE;
    DAQ_INFO("  Total size: " + std::to_string(_keySize));

    _spRtree.reset(DaqDB::RTreeEngine::Open(getOptions().pmem.poolPath,
                                            getOptions().pmem.totalSize,
                                            getOptions().pmem.allocUnitSize));
    if (_spRtree.get() == nullptr)
        throw OperationFailedException(errno, ::pmemobj_errormsg());

    size_t keySize = pmem()->SetKeySize(getOptions().key.size());
    if (keySize != getOptions().key.size()) {
        DAQ_INFO("Requested key size of " +
                 std::to_string(getOptions().key.size()) +
                 +"B does not match expected " + std::to_string(keySize) + "B");
        throw OperationFailedException(Status(NOT_SUPPORTED));
    }

    _spSpdk.reset(new SpdkCore(getOptions().offload));
    if ( _spSpdk->isSpdkReady() == true ) {
        DAQ_DEBUG("SPDK offload functionality is enabled");
    } else {
        DAQ_DEBUG("SPDK offload functionality is disabled");
    }

    _spDht.reset(new DhtCore(getOptions().dht));
    if (dhtCount) {
        _spDht->initNexus();
        _spDhtServer.reset(
            new DhtServer(getDhtCore(), this, dhtCount, baseCoreId));
        if (_spDhtServer->state == DhtServerState::DHT_SERVER_READY) {
            DAQ_DEBUG("DHT server started successfully");
        } else {
            DAQ_DEBUG("Can not start DHT server");
        }
        coresUsed += dhtCount;
        _spDht->initClient();
    }

    if ( _spSpdk->isSpdkReady() == true ) {
        _spOffloadPoller.reset(new DaqDB::OffloadPoller(pmem(), getSpdkCore(), baseCoreId + dhtCount, true));
        coresUsed++;
    }

    if ( _spOffloadPoller.get() )
        _spOffloadPoller->waitReady(); // synchronize until OffloadPoller is done initializing SPDK framework

    for (auto index = coresUsed; index < pollerCount + coresUsed; index++) {
        auto rqstPoller = new DaqDB::PmemPoller(pmem(), baseCoreId + index);
        if (_spSpdk->isSpdkReady() == true )
            rqstPoller->offloadPoller = _spOffloadPoller.get();
        _rqstPollers.push_back(rqstPoller);
    }

    _spPKey.reset(DaqDB::PrimaryKeyEngine::open(getOptions()));

    DAQ_DEBUG("KVStore initialization completed");
}

size_t KVStore::KeySize() { return _keySize; }

const Options &KVStore::getOptions() { return _options; }

void KVStore::LogMsg(std::string msg) {
    if (getOptions().runtime.logFunc) {
        getOptions().runtime.logFunc(msg);
    }
}

void KVStore::Put(const char *key, size_t keySize, char *value,
                  size_t valueSize, const PutOptions &options) {
    if (options.attr & PrimaryKeyAttribute::LONG_TERM)
        throw FUNC_NOT_IMPLEMENTED;

    /** @todo what if more values inserted for the same primary key? */
    pmem()->Put(key, value);
    try {
        pKey()->enqueueNext(Key(const_cast<char *>(key), keySize));
    } catch (OperationFailedException &e) {
        pmem()->Remove(key);
        throw;
    }
}

void KVStore::_getOffloaded(const char *key, size_t keySize, char *value,
                            size_t *valueSize) {
    if (!isOffloadEnabled())
        throw OperationFailedException(Status(OFFLOAD_DISABLED_ERROR));
    std::mutex mtx;
    std::condition_variable cv;
    bool ready = false;
    if (!_spOffloadPoller->enqueue(new OffloadRqst(
            OffloadOperation::GET, key, keySize, nullptr, 0,
            [&mtx, &cv, &ready, &value, &valueSize](
                KVStoreBase *kvs, Status status, const char *key,
                size_t keySize, const char *valueOff, size_t valueOffSize) {
                // todo add size check
                std::unique_lock<std::mutex> lck(mtx);
                std::memcpy(value, valueOff, valueOffSize);
                *valueSize = valueOffSize;
                ready = true;
                cv.notify_all();
            }))) {
        throw QueueFullException();
    }
    // wait for completion
    {
        std::unique_lock<std::mutex> lk(mtx);
        cv.wait_for(lk, 1s, [&ready] { return ready; });
    }
    if (!ready)
        throw OperationFailedException(Status(TIME_OUT));
}

void KVStore::_getOffloaded(const char *key, size_t keySize, char **value,
                            size_t *valueSize) {
    if (!isOffloadEnabled())
        throw OperationFailedException(Status(OFFLOAD_DISABLED_ERROR));
    std::mutex mtx;
    std::condition_variable cv;
    bool ready = false;
    if (!_spOffloadPoller->enqueue(new OffloadRqst(
            OffloadOperation::GET, key, keySize, nullptr, 0,
            [&mtx, &cv, &ready, &value, &valueSize](
                KVStoreBase *kvs, Status status, const char *key,
                size_t keySize, const char *valueOff, size_t valueOffSize) {
                *value = new char[valueOffSize];
                std::unique_lock<std::mutex> lck(mtx);
                std::memcpy(*value, valueOff, valueOffSize);
                *valueSize = valueOffSize;
                ready = true;
                cv.notify_all();
            }))) {
        throw QueueFullException();
    }
    // wait for completion
    {
        std::unique_lock<std::mutex> lk(mtx);
        cv.wait_for(lk, 1s, [&ready] { return ready; });
    }
    if (!ready)
        throw OperationFailedException(Status(TIME_OUT));
}

void KVStore::Get(const char *key, size_t keySize, char *value,
                  size_t *valueSize, const GetOptions &options) {
    size_t pValSize;
    char *pVal;
    uint8_t location;

    pmem()->Get(key, reinterpret_cast<void **>(&pVal), &pValSize, &location);
    if (!value) {
        DAQ_DEBUG("Error on get: value buffer is null");
        throw OperationFailedException(PMEM_ALLOCATION_ERROR);
    }
    if (location == PMEM) {
        if (*valueSize < pValSize) {
            DAQ_DEBUG("Error on get: buffer size " +
                      std::to_string(*valueSize) + " < value size " +
                      std::to_string(pValSize));
            throw OperationFailedException(EINVAL);
        }
        pmem_memcpy_nodrain(value, pVal, pValSize);
        *valueSize = pValSize;
    } else if (location == DISK) {
        _getOffloaded(key, keySize, value, valueSize);
    } else {
        throw OperationFailedException(EINVAL);
    }
}

void KVStore::Get(const char *key, size_t keySize, char **value,
                  size_t *valueSize, const GetOptions &options) {
    size_t pValSize;
    char *pVal;
    uint8_t location;

    pmem()->Get(key, reinterpret_cast<void **>(&pVal), &pValSize, &location);
    if (!value)
        throw OperationFailedException(PMEM_ALLOCATION_ERROR);
    if (location == PMEM) {
        *value = new char[pValSize];
        if (!value)
            throw OperationFailedException(ENOMEM);
        pmem_memcpy_nodrain(*value, pVal, pValSize);
        *valueSize = pValSize;
    } else if (location == DISK) {
        _getOffloaded(key, keySize, value, valueSize);
    } else {
        throw OperationFailedException(EINVAL);
    }
}

void KVStore::Update(const char *key, size_t keySize, char *value,
                     size_t valueSize, const UpdateOptions &options) {
    throw FUNC_NOT_IMPLEMENTED;
}

void KVStore::Update(const char *key, size_t keySize,
                     const UpdateOptions &options) {
    throw FUNC_NOT_IMPLEMENTED;
}

void KVStore::Remove(const char *key, size_t keySize) {
    size_t size;
    char *pVal;
    uint8_t location;

    pmem()->Get(key, reinterpret_cast<void **>(&pVal), &size, &location);

    if (location == DISK) {
        if (!isOffloadEnabled())
            throw OperationFailedException(Status(OFFLOAD_DISABLED_ERROR));

        std::mutex mtx;
        std::condition_variable cv;
        bool ready = false;
        if (!_spOffloadPoller->enqueue(new OffloadRqst(
                OffloadOperation::REMOVE, key, keySize, nullptr, 0,
                [&mtx, &cv, &ready](KVStoreBase *kvs, Status status,
                                    const char *key, size_t keySize,
                                    const char *value, size_t valueSize) {
                    std::unique_lock<std::mutex> lck(mtx);
                    ready = true;
                    cv.notify_all();
                }))) {
            throw QueueFullException();
        }

        // wait for completion
        {
            std::unique_lock<std::mutex> lk(mtx);
            cv.wait_for(lk, 1s, [&ready] { return ready; });
            if (!ready)
                throw OperationFailedException(Status(TIME_OUT));
        }

    } else {
        pmem()->Remove(key);
    }
}

void KVStore::Put(Key &&key, Value &&val, const PutOptions &options) {
    try {
        if (!getDhtCore()->isLocalKey(key)) {
            dhtClient()->put(key, val);
        } else {
            // todo add alloc if value is not buffered
            Put(key.data(), key.size(), val.data(), val.size());
        }
    } catch (...) {
        Free(key, std::move(val));
        Free(std::move(key));
        throw;
    }
    Free(key, std::move(val));
    Free(std::move(key));
}

void KVStore::PutAsync(Key &&key, Value &&value, KVStoreBaseCallback cb,
                       const PutOptions &options) {
    if (options.attr & PrimaryKeyAttribute::LONG_TERM) {
        Free(key, std::move(value));
        Free(std::move(key));
        throw FUNC_NOT_IMPLEMENTED;
    }

    thread_local int pollerId = 0;

    pollerId = (options.roundRobin()) ? ((pollerId + 1) % _rqstPollers.size())
                                      : options.pollerId();
    try {
        // todo memleak - key and value are not freed
        PmemRqst *msg = new PmemRqst(RqstOperation::PUT, key.data(), key.size(),
                                     value.data(), value.size(), cb);
        if (!_rqstPollers.at(pollerId)->enqueue(msg)) {
            delete msg;
            throw QueueFullException();
        }
    } catch (OperationFailedException &e) {
        cb(this, e.status(), key.data(), key.size(), value.data(),
           value.size());
    } catch (...) {
        throw;
    }
}

Value KVStore::Get(const Key &key, const GetOptions &options) {
    if (!getDhtCore()->isLocalKey(key))
        return dhtClient()->get(key);
    char *data;
    size_t size;
    Get(key.data(), key.size(), &data, &size, options);
    return Value(data, size);
}

void KVStore::GetAsync(const Key &key, KVStoreBaseCallback cb,
                       const GetOptions &options) {
    if (!getDhtCore()->isLocalKey(key))
        throw FUNC_NOT_IMPLEMENTED;
    if (options.attr & PrimaryKeyAttribute::LONG_TERM) {
        if (!isOffloadEnabled())
            throw OperationFailedException(Status(OFFLOAD_DISABLED_ERROR));

        try {
            if (!_spOffloadPoller->enqueue(
                    new OffloadRqst(OffloadOperation::GET, key.data(),
                                    key.size(), nullptr, 0, cb))) {
                throw QueueFullException();
            }
        } catch (OperationFailedException &e) {
            Value val;
            cb(this, e.status(), key.data(), key.size(), val.data(),
               val.size());
        }
    } else {
        thread_local int pollerId = 0;

        pollerId = (options.roundRobin())
                       ? ((pollerId + 1) % _rqstPollers.size())
                       : options.pollerId();

        if (!key.data()) {
            throw OperationFailedException(EINVAL);
        }
        try {
            if (!_rqstPollers.at(pollerId)->enqueue(
                    new PmemRqst(RqstOperation::GET, key.data(), key.size(),
                                 nullptr, 0, cb))) {
                throw QueueFullException();
            }
        } catch (OperationFailedException &e) {
            Value val;
            cb(this, e.status(), key.data(), key.size(), val.data(),
               val.size());
        }
    }
}

void KVStore::GetAny(char *key, size_t keySize, const GetOptions &options) {
    Key tmpKey = Key(key, keySize);
    pKey()->dequeueNext(tmpKey);
}

Key KVStore::GetAny(const AllocOptions &allocOptions,
                    const GetOptions &options) {
    Key key = AllocKey(allocOptions);
    try {
        pKey()->dequeueNext(key);
    } catch (...) {
        Free(std::move(key));
        throw;
    }
    return key;
}

void KVStore::GetAnyAsync(KVStoreBaseGetAnyCallback cb,
                          const AllocOptions &allocOptions,
                          const GetOptions &options) {
    throw FUNC_NOT_IMPLEMENTED;
}

void KVStore::Update(const Key &key, Value &&val,
                     const UpdateOptions &options) {
    if (options.attr & PrimaryKeyAttribute::LONG_TERM) {
        if (!isOffloadEnabled())
            throw OperationFailedException(Status(OFFLOAD_DISABLED_ERROR));

        std::mutex mtx;
        std::condition_variable cv;
        bool ready = false;

        if (!_spOffloadPoller->enqueue(new OffloadRqst(
                OffloadOperation::UPDATE, key.data(), key.size(), val.data(),
                val.size(),
                [&mtx, &cv, &ready](KVStoreBase *kvs, Status status,
                                    const char *key, size_t keySize,
                                    const char *value, size_t valueSize) {
                    std::unique_lock<std::mutex> lck(mtx);
                    ready = true;
                    cv.notify_all();
                }))) {
            throw QueueFullException();
        }
        // wait for completion
        {
            std::unique_lock<std::mutex> lk(mtx);
            cv.wait_for(lk, 1s, [&ready] { return ready; });
            if (!ready)
                throw OperationFailedException(Status(TIME_OUT));
        }

    } else {
        // @TODO other attributes not supported by rtree currently
        throw FUNC_NOT_IMPLEMENTED;
    }
}

void KVStore::Update(const Key &key, const UpdateOptions &options) {
    Value emptyVal;
    Update(key, std::move(emptyVal), options);
}

void KVStore::UpdateAsync(const Key &key, Value &&value, KVStoreBaseCallback cb,
                          const UpdateOptions &options) {
    if (options.attr & PrimaryKeyAttribute::LONG_TERM) {
        if (!isOffloadEnabled())
            throw OperationFailedException(Status(OFFLOAD_DISABLED_ERROR));

        try {
            if (!_spOffloadPoller->enqueue(new OffloadRqst(
                    OffloadOperation::UPDATE, key.data(), key.size(),
                    value.data(), value.size(), cb))) {
                throw QueueFullException();
            }
        } catch (OperationFailedException &e) {
            Value val;
            cb(this, e.status(), key.data(), key.size(), val.data(),
               val.size());
        }
    } else {
        // @TODO other attributes not supported by rtree currently
        throw FUNC_NOT_IMPLEMENTED;
    }
}

void KVStore::UpdateAsync(const Key &key, const UpdateOptions &options,
                          KVStoreBaseCallback cb) {
    Value emptyVal;
    UpdateAsync(key, std::move(emptyVal), cb, options);
}

std::vector<KVPair> KVStore::GetRange(const Key &beg, const Key &end,
                                      const GetOptions &options) {
    throw FUNC_NOT_IMPLEMENTED;
}

void KVStore::GetRangeAsync(const Key &beg, const Key &end,
                            KVStoreBaseRangeCallback cb,
                            const GetOptions &options) {
    throw FUNC_NOT_IMPLEMENTED;
}

void KVStore::Remove(const Key &key) {
    if (!getDhtCore()->isLocalKey(key))
        return dhtClient()->remove(key);
    Remove(key.data(), key.size());
}

void KVStore::RemoveRange(const Key &beg, const Key &end) {
    throw FUNC_NOT_IMPLEMENTED;
}

void KVStore::Alloc(const char *key, size_t keySize, char **value, size_t size,
                    const AllocOptions &options) {
    if (options.attr & KeyValAttribute::KVS_BUFFERED)
        pmem()->AllocValueForKey(key, size, value);
    else
        *value = new char[size];
}

Value KVStore::Alloc(const Key &key, size_t size, const AllocOptions &options) {
    auto valAttr = KeyValAttribute::NOT_BUFFERED;
    if (options.attr & KeyValAttribute::KVS_BUFFERED) {
        valAttr = KeyValAttribute::KVS_BUFFERED;
        if (!getDhtCore()->isLocalKey(key))
            return dhtClient()->alloc(key, size);
    }
    char *value;
    Alloc(key.data(), key.size(), &value, size, options);
    return Value(value, size, valAttr);
}

void KVStore::Free(const Key &key, Value &&value) {
    if (value.isKvsBuffered()) {
        if (!getDhtCore()->isLocalKey(key))
            return dhtClient()->free(key, std::move(value));
        // todo add pmem free method (free only if not in use)
    } else {
        delete[] value.data();
    }
}

void KVStore::Realloc(const Key &key, Value &value, size_t size,
                      const AllocOptions &options) {
    throw FUNC_NOT_IMPLEMENTED;
}

void KVStore::ChangeOptions(Value &value, const AllocOptions &options) {
    std::unique_lock<std::mutex> l(_lock);

    throw FUNC_NOT_IMPLEMENTED;
}

Key KVStore::AllocKey(const AllocOptions &options) {
    if (options.attr & KeyValAttribute::KVS_BUFFERED) {
        return dhtClient()->allocKey(KeySize());
    } else {
        return Key(new char[KeySize()], KeySize());
    }
}

void KVStore::Free(Key &&key) {
    if (key.isKvsBuffered()) {
        dhtClient()->free(std::move(key));
    } else {
        delete[] key.data();
    }
}

void KVStore::ChangeOptions(Key &key, const AllocOptions &options) {
    std::unique_lock<std::mutex> l(_lock);

    throw FUNC_NOT_IMPLEMENTED;
}

bool KVStore::IsOffloaded(Key &key) {
    bool result = false;
    ValCtx valCtx;
    try {
        pmem()->Get(key.data(), key.size(), &valCtx.val, &valCtx.size,
                    &valCtx.location);
        result = (valCtx.location == LOCATIONS::DISK);
    } catch (OperationFailedException &e) {
        result = false;
    }
    return result;
}
std::string KVStore::getProperty(const std::string &name) {
    std::unique_lock<std::mutex> l(_lock);

    if (name == "daqdb.dht.id")
        return std::to_string(_spDhtServer->getId());
    if (name == "daqdb.dht.neighbours")
        return _spDhtServer->printNeighbors();
    if (name == "daqdb.dht.status") {
        std::stringstream result;
        result << _spDhtServer->printStatus() << std::endl;
        return result.str();
    }
    if (name == "daqdb.dht.ip")
        return _spDhtServer->getIp();
    if (name == "daqdb.dht.port")
        return std::to_string(_spDhtServer->getPort());
    if (name == "daqdb.pmem.path")
        return getOptions().pmem.poolPath;
    if (name == "daqdb.pmem.size")
        return std::to_string(getOptions().pmem.totalSize);
    if (name == "daqdb.pmem.alloc_unit_size")
        return std::to_string(getOptions().pmem.allocUnitSize);

    return "";
}

uint64_t KVStore::GetTreeSize() { return pmem()->GetTreeSize(); }

uint8_t KVStore::GetTreeDepth() { return pmem()->GetTreeDepth(); }

uint64_t KVStore::GetLeafCount() { return pmem()->GetLeafCount(); }

} // namespace DaqDB
