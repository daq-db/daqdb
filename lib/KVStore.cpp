/**
 * Copyright 2017-2018 Intel Corporation.
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

#include <sstream>
#include <chrono>
#include <condition_variable>

#include <DhtUtils.h>
#include <Logger.h>
#include <daqdb/Types.h>

/** @TODO jradtke: should be taken from configuration file */
#define POOLER_CPU_CORE_BASE 1

using namespace std::chrono_literals;

namespace DaqDB {

KVStoreBase *KVStoreBase::Open(const Options &options) {
    return KVStore::Open(options);
}

KVStoreBase *KVStore::Open(const Options &options) {
    KVStore *kvs = new KVStore(options);

    kvs->init();

    return kvs;
}

KVStore::KVStore(const Options &options)
    : env(options, this), _offloadPooler(nullptr), _offloadReactor(nullptr) {}

KVStore::~KVStore() {
    _dht.reset();

    DaqDB::RTreeEngine::Close(_rtree.get());
    _rtree.reset();

    for (auto index = 0; index < _rqstPoolers.size(); index++) {
        delete _rqstPoolers.at(index);
    }
}

const Options &KVStore::getOptions() { return env.getOptions(); }
size_t KVStore::KeySize() { return env.keySize(); }

void KVStore::LogMsg(std::string msg) {
    if (getOptions().Runtime.logFunc) {
        getOptions().Runtime.logFunc(msg);
    }
}

void KVStore::Put(Key &&key, Value &&val, const PutOptions &options) {
    if (options.Attr & PrimaryKeyAttribute::LONG_TERM) {
        throw FUNC_NOT_IMPLEMENTED;
    }
    // @TODO jradtke REMOTE attribute will be remove
    if (options.Attr & PrimaryKeyAttribute::REMOTE) {
        return _dht->Put(key, val);
    }

    StatusCode rc = _rtree->Put(key.data(), val.data());
    // Free(std::move(val)); /** @TODO jschmieg: free value if needed */
    if (rc != StatusCode::Ok)
        throw OperationFailedException(EINVAL);
}

void KVStore::PutAsync(Key &&key, Value &&value, KVStoreBaseCallback cb,
                       const PutOptions &options) {
    if (options.Attr & PrimaryKeyAttribute::LONG_TERM) {
        throw FUNC_NOT_IMPLEMENTED;
    }
    thread_local int poolerId = 0;

    poolerId = (options.roundRobin()) ? ((poolerId + 1) % _rqstPoolers.size())
                                      : options.poolerId();

    if (!key.data()) {
        throw OperationFailedException(EINVAL);
    }
    try {
        PmemRqst *msg = new PmemRqst(RqstOperation::PUT, key.data(), key.size(),
                                     value.data(), value.size(), cb);
        if (!_rqstPoolers.at(poolerId)->Enqueue(msg)) {
            throw QueueFullException();
        }
    } catch (OperationFailedException &e) {
        cb(this, e.status(), key.data(), key.size(), value.data(),
           value.size());
    }
}

Value KVStore::Get(const Key &key, const GetOptions &options) {

    size_t size;
    char *pVal;
    uint8_t location;

    if (options.Attr & PrimaryKeyAttribute::REMOTE) {
        return _dht->Get(key);
    }

    StatusCode rc = _rtree->Get(key.data(), reinterpret_cast<void **>(&pVal),
                                &size, &location);
    if (rc != StatusCode::Ok || !pVal) {
        if (rc == StatusCode::KeyNotFound) {
            throw OperationFailedException(Status(KeyNotFound));
        } else {
            throw OperationFailedException(EINVAL);
        }
    }
    if (location == PMEM) {
        Value value(new char[size], size);
        std::memcpy(value.data(), pVal, size);
        return value;
    } else if (location == DISK) {
        if (!_offloadReactor->isEnabled())
            throw OperationFailedException(Status(OffloadDisabledError));

        Value *resultValue = nullptr;
        std::mutex mtx;
        std::condition_variable cv;
        bool ready = false;
        if (!_offloadPooler->Enqueue(new OffloadRqst(
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
            throw OperationFailedException(Status(TimeOutError));

        return *resultValue;
    } else {
        throw OperationFailedException(EINVAL);
    }
}

void KVStore::GetAsync(const Key &key, KVStoreBaseCallback cb,
                       const GetOptions &options) {
    if (options.Attr & PrimaryKeyAttribute::LONG_TERM) {
        if (!_offloadReactor->isEnabled())
            throw OperationFailedException(Status(OffloadDisabledError));

        try {
            if (!_offloadPooler->Enqueue(new OffloadRqst(OffloadOperation::GET,
                                                         key.data(), key.size(),
                                                         nullptr, 0, cb))) {
                throw QueueFullException();
            }
        } catch (OperationFailedException &e) {
            Value val;
            cb(this, e.status(), key.data(), key.size(), val.data(),
               val.size());
        }
    } else {
        thread_local int poolerId = 0;

        poolerId = (options.roundRobin())
                       ? ((poolerId + 1) % _rqstPoolers.size())
                       : options.poolerId();

        if (!key.data()) {
            throw OperationFailedException(EINVAL);
        }
        try {
            if (!_rqstPoolers.at(poolerId)->Enqueue(
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

Key KVStore::GetAny(const GetOptions &options) { throw FUNC_NOT_IMPLEMENTED; }

void KVStore::GetAnyAsync(KVStoreBaseGetAnyCallback cb,
                          const GetOptions &options) {
    throw FUNC_NOT_IMPLEMENTED;
}

void KVStore::Update(const Key &key, Value &&val,
                     const UpdateOptions &options) {
    if (options.Attr & PrimaryKeyAttribute::LONG_TERM) {
        if (!_offloadReactor->isEnabled())
            throw OperationFailedException(Status(OffloadDisabledError));

        std::mutex mtx;
        std::condition_variable cv;
        bool ready = false;

        if (!_offloadPooler->Enqueue(new OffloadRqst(
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
                throw OperationFailedException(Status(TimeOutError));
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
    if (options.Attr & PrimaryKeyAttribute::LONG_TERM) {
        if (!_offloadReactor->isEnabled())
            throw OperationFailedException(Status(OffloadDisabledError));

        try {
            if (!_offloadPooler->Enqueue(new OffloadRqst(
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

    size_t size;
    char *pVal;
    uint8_t location;

    StatusCode rc = _rtree->Get(key.data(), reinterpret_cast<void **>(&pVal),
                                &size, &location);
    if (rc != StatusCode::Ok || !pVal) {
        if (rc == StatusCode::KeyNotFound)
            throw OperationFailedException(Status(KeyNotFound));
        throw OperationFailedException(EINVAL);
    }

    if (location == DISK) {
        if (!_offloadReactor->isEnabled())
            throw OperationFailedException(Status(OffloadDisabledError));

        std::mutex mtx;
        std::condition_variable cv;
        bool ready = false;
        if (!_offloadPooler->Enqueue(new OffloadRqst(
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
                throw OperationFailedException(Status(TimeOutError));
        }

    } else {
        StatusCode rc = _rtree->Remove(key.data());
        if (rc != StatusCode::Ok) {
            if (rc == StatusCode::KeyNotFound)
                throw OperationFailedException(KeyNotFound);

            throw OperationFailedException(EINVAL);
        }
    }
}

void KVStore::RemoveRange(const Key &beg, const Key &end) {
    throw FUNC_NOT_IMPLEMENTED;
}

Value KVStore::Alloc(const Key &key, size_t size, const AllocOptions &options) {
    char *val = nullptr;
    StatusCode rc = _rtree->AllocValueForKey(key.data(), size, &val);
    if (rc != StatusCode::Ok) {
        throw OperationFailedException(AllocationError);
    }
    assert(val);
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
    return Key(new char[env.keySize()], env.keySize());
}

void KVStore::Free(Key &&key) { delete[] key.data(); }

void KVStore::ChangeOptions(Key &key, const AllocOptions &options) {
    std::unique_lock<std::mutex> l(_lock);

    throw FUNC_NOT_IMPLEMENTED;
}

bool KVStore::IsOffloaded(Key &key) {
    bool result = false;

    ValCtx valCtx;
    StatusCode rc = _rtree->Get(key.data(), key.size(), &valCtx.val,
                                &valCtx.size, &valCtx.location);
    if (rc != StatusCode::Ok) {
        if (rc == StatusCode::KeyNotFound)
            throw OperationFailedException(KeyNotFound);

        throw OperationFailedException(EINVAL);
    }
    return (valCtx.location == LOCATIONS::DISK);
}

std::string KVStore::getProperty(const std::string &name) {
    std::unique_lock<std::mutex> l(_lock);

    if (name == "daqdb.dht.id")
        return std::to_string(_dht->getDhtId());
    if (name == "daqdb.dht.neighbours")
        return _dht->printNeighbors();
    if (name == "daqdb.dht.status") {
        std::stringstream result;
        result << _dht->printStatus() << endl;
        result << _offloadReactor->printStatus() << endl;
        return result.str();
    }
    if (name == "daqdb.dht.ip")
        return _dht->getIp();
    if (name == "daqdb.dht.port")
        return std::to_string(_dht->getPort());
    if (name == "daqdb.pmem.path")
        return getOptions().PMEM.poolPath;
    if (name == "daqdb.pmem.size")
        return std::to_string(getOptions().PMEM.totalSize);
    if (name == "daqdb.pmem.alloc_unit_size")
        return std::to_string(getOptions().PMEM.allocUnitSize);

    return "";
}

void KVStore::init() {

    auto poolerCount = getOptions().Runtime.numOfPoolers;

    if (getOptions().Runtime.logFunc)
        gLog.setLogFunc(getOptions().Runtime.logFunc);

    env.createSpdkConfFiles();
    _offloadReactor = new DaqDB::OffloadReactor(
        POOLER_CPU_CORE_BASE + poolerCount + 1, env.getSpdkConfFile(), [&]() {
            // @TODO jradtke move to separate function/wrapper
            getOptions().Runtime.shutdownFunc();
        });

    while (_offloadReactor->state == ReactorState::REACTOR_INIT) {
        sleep(1);
    }

    env.removeSpdkConfFiles();
    if (_offloadReactor->isEnabled()) {
        DAQ_DEBUG("SPDK offload functionality is enabled");
    } else {
        DAQ_DEBUG("SPDK offload functionality is disabled");
    }

    auto dhtPort =
        getOptions().Dht.port ?: DaqDB::utils::getFreePort(env.ioService(), 0);

    _rtree.reset(DaqDB::RTreeEngine::Open(getOptions().PMEM.poolPath,
                                          getOptions().PMEM.totalSize,
                                          getOptions().PMEM.allocUnitSize));
    if (_rtree == nullptr)
        throw OperationFailedException(errno, ::pmemobj_errormsg());

    if (_offloadReactor->isEnabled()) {
        _offloadPooler =
            new DaqDB::OffloadPooler(_rtree, _offloadReactor->bdevCtx,
                                     getOptions().Offload.allocUnitSize);
        _offloadPooler->InitFreeList();
        _offloadReactor->RegisterPooler(_offloadPooler);
    }

    for (auto index = 0; index < poolerCount; index++) {
        auto rqstPooler =
            new DaqDB::PmemPooler(_rtree, POOLER_CPU_CORE_BASE + index);
        if (_offloadReactor->isEnabled())
            rqstPooler->offloadPooler = _offloadPooler;
        _rqstPoolers.push_back(rqstPooler);
    }

    _dht.reset(new DaqDB::ZhtNode(env.ioService(), &env, dhtPort));

    DAQ_DEBUG("KVStoreBaseImpl initialization completed");
}

} // namespace DaqDB
