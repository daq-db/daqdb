/**
 * Copyright 2017-2019 Intel Corporation.
 *
 * This software and the related documents are Intel copyrighted materials,
 * and your use of them is governed by the express license under which they
 * were provided to you (Intel OBL Internal Use License).
 * Unless the License provides otherwise, you may not use, modify, copy,
 * publish, distribute, disclose or transmit this software or the related
 * documents without Intel's prior written permission.
 *
 * This software and the related documents are provided as is, with no
 * express or implied warranties, other than those that are expressly
 * stated in the License.
 */

#include "KVStore.h"

#include <boost/filesystem.hpp>
#include <chrono>
#include <condition_variable>
#include <sstream>

#include <DhtUtils.h>
#include <Logger.h>
#include <daqdb/Types.h>

/** @TODO jradtke: should be taken from configuration file */
#define POLLER_CPU_CORE_BASE 1

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

    for (auto index = 0; index < _rqstPollers.size(); index++) {
        delete _rqstPollers.at(index);
    }

    _spOffloadPoller.reset();
}

void KVStore::init() {
    auto pollerCount = getOptions().runtime.numOfPollers;
    auto coreUsed = 0;

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

    // @TODO jradtke Temporary workaround - ARTree rebuilding not implemented
    if (bf::exists(getOptions().pmem.poolPath)) {
        bf::remove(getOptions().pmem.poolPath);
    }
    _spRtree.reset(DaqDB::RTreeEngine::Open(getOptions().pmem.poolPath,
                                            getOptions().pmem.totalSize,
                                            getOptions().pmem.allocUnitSize));
    if (_spRtree.get() == nullptr)
        throw OperationFailedException(errno, ::pmemobj_errormsg());

    _spSpdk.reset(new SpdkCore(getOptions().offload));
    if (isOffloadEnabled()) {
        DAQ_DEBUG("SPDK offload functionality is enabled");
    } else {
        DAQ_DEBUG("SPDK offload functionality is disabled");
    }

    _spDht.reset(new DhtCore(getOptions().dht));
    _spDhtServer.reset(new DhtServer(getDhtCore(),
                                     static_cast<KVStoreBase *>(this),
                                     getDhtCore()->getLocalNode()->getPort()));
    if (_spDhtServer->state == DhtServerState::DHT_SERVER_READY) {
        DAQ_DEBUG("DHT server started successfully");
        _spDht->initClient();
        if (_spDht->getClient()->state == DhtClientState::DHT_CLIENT_READY) {
            DAQ_DEBUG("DHT client started successfully");
        } else {
            DAQ_DEBUG("Can not start DHT client");
        }
    } else {
        DAQ_DEBUG("Can not start DHT server");
        DAQ_DEBUG("Can not start DHT client");
        _spDht->getClient()->state = DhtClientState::DHT_CLIENT_ERROR;
    }

    if (isOffloadEnabled()) {
        _spOffloadPoller.reset(new DaqDB::OffloadPoller(
            pmem(), getSpdkCore(), POLLER_CPU_CORE_BASE + coreUsed));
        coreUsed++;
        _spOffloadPoller->initFreeList();
    }

    for (auto index = coreUsed; index < pollerCount + coreUsed; index++) {
        auto rqstPoller =
            new DaqDB::PmemPoller(pmem(), POLLER_CPU_CORE_BASE + index);
        if (isOffloadEnabled())
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

void KVStore::Put(Key &&key, Value &&val, const PutOptions &options) {
    if (options.attr & PrimaryKeyAttribute::LONG_TERM) {
        Free(std::move(key));
        throw FUNC_NOT_IMPLEMENTED;
    }

    try {
        if (!getDhtCore()->isLocalKey(key))
            return dhtClient()->put(key, val);

        /** @todo what if more values inserted for the same primary key? */
        char *keyBuff = key.data();
        pmem()->Put(keyBuff, val.data());
        try {
            pKey()->enqueueNext(key);
        }
        catch (OperationFailedException &e) {
            pmem()->Remove(keyBuff);
            throw;
        }
    }
    catch (...) {
        Free(std::move(key));
        throw;
    }
    Free(std::move(key));
    // Free(std::move(val)); /** @TODO jschmieg: free value if needed */
}

void KVStore::PutAsync(Key &&key, Value &&value, KVStoreBaseCallback cb,
                       const PutOptions &options) {
    if (options.attr & PrimaryKeyAttribute::LONG_TERM) {
        Free(std::move(key));
        throw FUNC_NOT_IMPLEMENTED;
    }
    thread_local int pollerId = 0;

    pollerId = (options.roundRobin()) ? ((pollerId + 1) % _rqstPollers.size())
                                      : options.pollerId();

    if (!key.data()) {
        throw OperationFailedException(EINVAL);
    }
    try {
        // todo memleak - key is not freed
        PmemRqst *msg = new PmemRqst(RqstOperation::PUT, key.data(), key.size(),
                                     value.data(), value.size(), cb);
        if (!_rqstPollers.at(pollerId)->enqueue(msg)) {
            throw QueueFullException();
        }
    }
    catch (OperationFailedException &e) {
        cb(this, e.status(), key.data(), key.size(), value.data(),
           value.size());
        Free(std::move(key));
    }
}

Value KVStore::Get(const Key &key, const GetOptions &options) {

    size_t size;
    char *pVal;
    uint8_t location;

    if (!getDhtCore()->isLocalKey(key)) {
        return dhtClient()->get(key);
    }

    pmem()->Get(key.data(), reinterpret_cast<void **>(&pVal), &size, &location);
    if (location == PMEM) {
        Value value(new char[size], size);
        std::memcpy(value.data(), pVal, size);
        return value;
    } else if (location == DISK) {
        if (!isOffloadEnabled())
            throw OperationFailedException(Status(OFFLOAD_DISABLED_ERROR));

        Value *resultValue = nullptr;
        std::mutex mtx;
        std::condition_variable cv;
        bool ready = false;
        if (!_spOffloadPoller->enqueue(new OffloadRqst(
                 OffloadOperation::GET, key.data(), key.size(), nullptr, 0,
                 [&mtx, &cv, &ready, &resultValue](
                     KVStoreBase *kvs, Status status, const char *key,
                     size_t keySize, const char *value, size_t valueSize) {
                     std::unique_lock<std::mutex> lck(mtx);

                     resultValue = new Value(new char[valueSize], valueSize);
                     std::memcpy(resultValue->data(), value, valueSize);
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
        if (resultValue == nullptr)
            throw OperationFailedException(Status(TIME_OUT));

        return *resultValue;
    } else {
        throw OperationFailedException(EINVAL);
    }
}

void KVStore::GetAsync(const Key &key, KVStoreBaseCallback cb,
                       const GetOptions &options) {
    if (options.attr & PrimaryKeyAttribute::LONG_TERM) {
        if (!isOffloadEnabled())
            throw OperationFailedException(Status(OFFLOAD_DISABLED_ERROR));

        try {
            if (!_spOffloadPoller->enqueue(
                     new OffloadRqst(OffloadOperation::GET, key.data(),
                                     key.size(), nullptr, 0, cb))) {
                throw QueueFullException();
            }
        }
        catch (OperationFailedException &e) {
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
            if (!_rqstPollers.at(pollerId)
                     ->enqueue(new PmemRqst(RqstOperation::GET, key.data(),
                                            key.size(), nullptr, 0, cb))) {
                throw QueueFullException();
            }
        }
        catch (OperationFailedException &e) {
            Value val;
            cb(this, e.status(), key.data(), key.size(), val.data(),
               val.size());
        }
    }
}

Key KVStore::GetAny(const GetOptions &options) {
    Key key = AllocKey();
    try {
        pKey()->dequeueNext(key);
    }
    catch (...) {
        Free(std::move(key));
    }
    return key;
}

void KVStore::GetAnyAsync(KVStoreBaseGetAnyCallback cb,
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
        }
        catch (OperationFailedException &e) {
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

    size_t size;
    char *pVal;
    uint8_t location;

    if (!getDhtCore()->isLocalKey(key)) {
        return dhtClient()->remove(key);
    }

    pmem()->Get(key.data(), reinterpret_cast<void **>(&pVal), &size, &location);

    if (location == DISK) {
        if (!isOffloadEnabled())
            throw OperationFailedException(Status(OFFLOAD_DISABLED_ERROR));

        std::mutex mtx;
        std::condition_variable cv;
        bool ready = false;
        if (!_spOffloadPoller->enqueue(new OffloadRqst(
                 OffloadOperation::REMOVE, key.data(), key.size(), nullptr, 0,
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
        pmem()->Remove(key.data());
    }
}

void KVStore::RemoveRange(const Key &beg, const Key &end) {
    throw FUNC_NOT_IMPLEMENTED;
}

Value KVStore::Alloc(const Key &key, size_t size, const AllocOptions &options) {
    char *val = nullptr;
    pmem()->AllocValueForKey(key.data(), size, &val);
    return Value(val, size);
}

void KVStore::Free(Value &&value) { delete[] value.data(); }

void KVStore::Realloc(Value &value, size_t size, const AllocOptions &options) {
    throw FUNC_NOT_IMPLEMENTED;
}

void KVStore::ChangeOptions(Value &value, const AllocOptions &options) {
    std::unique_lock<std::mutex> l(_lock);

    throw FUNC_NOT_IMPLEMENTED;
}

Key KVStore::AllocKey(const AllocOptions &options) {
    return dhtClient()->allocKey(KeySize());
}

void KVStore::Free(Key &&key) { dhtClient()->free(std::move(key)); }

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
    }
    catch (OperationFailedException &e) {
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

} // namespace DaqDB
