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

#include <chrono>
#include <condition_variable>

#include "../debug/Logger.h"
#include "../dht/DhtUtils.h"
#include "KVStoreBaseImpl.h"
#include "common.h"
#include <FogKV/Types.h>

#include <json/json.h>

/** @TODO jradtke: should be taken from configuration file */
#define POOLER_CPU_CORE_BASE 1

using namespace std::chrono_literals;

namespace FogKV {

KVStoreBase *KVStoreBase::Open(const Options &options) {
    return KVStoreBaseImpl::Open(options);
}

KVStoreBase *KVStoreBaseImpl::Open(const Options &options) {
    KVStoreBaseImpl *kvs = new KVStoreBaseImpl(options);

    kvs->init();

    return kvs;
}

size_t KVStoreBaseImpl::KeySize() {
    std::unique_lock<std::mutex> l(mLock);
    return mKeySize;
}

const Options &KVStoreBaseImpl::getOptions() { return mOptions; }

void KVStoreBaseImpl::LogMsg(std::string msg) {
    if (getOptions().Runtime.logFunc) {
        getOptions().Runtime.logFunc(msg);
    }
}

void KVStoreBaseImpl::Put(Key &&key, Value &&val, const PutOptions &options) {
    if (options.Attr & PrimaryKeyAttribute::LONG_TERM) {
        throw FUNC_NOT_IMPLEMENTED;
    }

    StatusCode rc = mRTree->Put(key.data(), val.data());
    // Free(std::move(val)); /** @TODO jschmieg: free value if needed */
    if (rc != StatusCode::Ok)
        throw OperationFailedException(EINVAL);
}

void KVStoreBaseImpl::PutAsync(Key &&key, Value &&value, KVStoreBaseCallback cb,
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
        RqstMsg *msg = new RqstMsg(RqstOperation::PUT, key.data(), key.size(),
                                   value.data(), value.size(), cb);
        if (!_rqstPoolers.at(poolerId)->EnqueueMsg(msg)) {
            throw QueueFullException();
        }
    } catch (OperationFailedException &e) {
        cb(this, e.status(), key.data(), key.size(), value.data(),
           value.size());
    }
}

Value KVStoreBaseImpl::Get(const Key &key, const GetOptions &options) {

    size_t size;
    char *pVal;
    uint8_t location;

    StatusCode rc = mRTree->Get(key.data(), reinterpret_cast<void **>(&pVal),
                                &size, &location);
    if (rc != StatusCode::Ok || !pVal) {
        if (rc == StatusCode::KeyNotFound)
            throw OperationFailedException(Status(KeyNotFound));

        throw OperationFailedException(EINVAL);
    }
    if (location == PMEM) {
        Value value(new char[size], size);
        std::memcpy(value.data(), pVal, size);
        return value;
    } else if (location == DISK) {
        if (!_offloadEnabled)
            throw OperationFailedException(Status(OffloadDisabledError));

        Value *resultValue = nullptr;
        std::mutex mtx;
        std::condition_variable cv;
        bool ready = false;
        if (!_offloadPooler->EnqueueMsg(new OffloadRqstMsg(
                OffloadRqstOperation::GET, key.data(), key.size(), nullptr, 0,
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

void KVStoreBaseImpl::GetAsync(const Key &key, KVStoreBaseCallback cb,
                               const GetOptions &options) {
    if (options.Attr & PrimaryKeyAttribute::LONG_TERM) {
        if (!_offloadEnabled)
            throw OperationFailedException(Status(OffloadDisabledError));

        try {
            if (!_offloadPooler->EnqueueMsg(
                    new OffloadRqstMsg(OffloadRqstOperation::GET, key.data(),
                                       key.size(), nullptr, 0, cb))) {
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
            if (!_rqstPoolers.at(poolerId)->EnqueueMsg(
                    new RqstMsg(RqstOperation::GET, key.data(), key.size(),
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

Key KVStoreBaseImpl::GetAny(const GetOptions &options) {
    throw FUNC_NOT_IMPLEMENTED;
}

void KVStoreBaseImpl::GetAnyAsync(KVStoreBaseGetAnyCallback cb,
                                  const GetOptions &options) {
    throw FUNC_NOT_IMPLEMENTED;
}

void KVStoreBaseImpl::Update(const Key &key, Value &&val,
                             const UpdateOptions &options) {
    if (options.Attr & PrimaryKeyAttribute::LONG_TERM) {
        if (!_offloadEnabled)
            throw OperationFailedException(Status(OffloadDisabledError));

        std::mutex mtx;
        std::condition_variable cv;
        bool ready = false;
        if (!_offloadPooler->EnqueueMsg(new OffloadRqstMsg(
                OffloadRqstOperation::UPDATE, key.data(), key.size(),
                val.data(), val.size(),
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
            cv.wait_for(lk, 10s, [&ready] { return ready; });
            if (!ready)
                throw OperationFailedException(Status(TimeOutError));
        }

    } else {
        // @TODO other attributes not supported by rtree currently
        throw FUNC_NOT_IMPLEMENTED;
    }
}

void KVStoreBaseImpl::Update(const Key &key, const UpdateOptions &options) {
    Value emptyVal;
    Update(key, std::move(emptyVal), options);
}

void KVStoreBaseImpl::UpdateAsync(const Key &key, Value &&value,
                                  KVStoreBaseCallback cb,
                                  const UpdateOptions &options) {
    if (options.Attr & PrimaryKeyAttribute::LONG_TERM) {
        if (!_offloadEnabled)
            throw OperationFailedException(Status(OffloadDisabledError));

        try {
            if (!_offloadPooler->EnqueueMsg(new OffloadRqstMsg(
                    OffloadRqstOperation::UPDATE, key.data(), key.size(),
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

void KVStoreBaseImpl::UpdateAsync(const Key &key, const UpdateOptions &options,
                                  KVStoreBaseCallback cb) {
    Value emptyVal;
    UpdateAsync(key, std::move(emptyVal), cb, options);
}

std::vector<KVPair> KVStoreBaseImpl::GetRange(const Key &beg, const Key &end,
                                              const GetOptions &options) {
    throw FUNC_NOT_IMPLEMENTED;
}

void KVStoreBaseImpl::GetRangeAsync(const Key &beg, const Key &end,
                                    KVStoreBaseRangeCallback cb,
                                    const GetOptions &options) {
    throw FUNC_NOT_IMPLEMENTED;
}

void KVStoreBaseImpl::Remove(const Key &key) {

    size_t size;
    char *pVal;
    uint8_t location;

    StatusCode rc = mRTree->Get(key.data(), reinterpret_cast<void **>(&pVal),
                                &size, &location);
    if (rc != StatusCode::Ok || !pVal) {
        if (rc == StatusCode::KeyNotFound)
            throw OperationFailedException(Status(KeyNotFound));
        throw OperationFailedException(EINVAL);
    }

    if (location == DISK) {
        if (!_offloadEnabled)
            throw OperationFailedException(Status(OffloadDisabledError));

        std::mutex mtx;
        std::condition_variable cv;
        bool ready = false;
        if (!_offloadPooler->EnqueueMsg(new OffloadRqstMsg(
                OffloadRqstOperation::REMOVE, key.data(), key.size(), nullptr,
                0,
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
            cv.wait_for(lk, 10s, [&ready] { return ready; });
            if (!ready)
                throw OperationFailedException(Status(TimeOutError));
        }

    } else {
        StatusCode rc = mRTree->Remove(key.data());
        if (rc != StatusCode::Ok) {
            if (rc == StatusCode::KeyNotFound)
                throw OperationFailedException(KeyNotFound);

            throw OperationFailedException(EINVAL);
        }
    }
}

void KVStoreBaseImpl::RemoveRange(const Key &beg, const Key &end) {
    throw FUNC_NOT_IMPLEMENTED;
}

Value KVStoreBaseImpl::Alloc(const Key &key, size_t size,
                             const AllocOptions &options) {
    char *val = nullptr;
    mRTree->AllocValueForKey(key.data(), size, &val);
    if (!val) {
        throw OperationFailedException(AllocationError);
    }
    return Value(val, size);
}

void KVStoreBaseImpl::Free(Value &&value) { delete[] value.data(); }

void KVStoreBaseImpl::Realloc(Value &value, size_t size,
                              const AllocOptions &options) {
    throw FUNC_NOT_IMPLEMENTED;
}

void KVStoreBaseImpl::ChangeOptions(Value &value, const AllocOptions &options) {
    std::unique_lock<std::mutex> l(mLock);

    throw FUNC_NOT_IMPLEMENTED;
}

Key KVStoreBaseImpl::AllocKey(const AllocOptions &options) {
    return Key(new char[mKeySize], mKeySize);
}

void KVStoreBaseImpl::Free(Key &&key) { delete[] key.data(); }

void KVStoreBaseImpl::ChangeOptions(Key &key, const AllocOptions &options) {
    std::unique_lock<std::mutex> l(mLock);

    throw FUNC_NOT_IMPLEMENTED;
}

KVStoreBaseImpl::KVStoreBaseImpl(const Options &options)
    : mOptions(options), mKeySize(0), _offloadPooler(nullptr),
      _offloadReactor(nullptr) {
    _io_service = new asio::io_service();

    for (size_t i = 0; i < options.Key.nfields(); i++)
        mKeySize += options.Key.field(i).Size;
}

KVStoreBaseImpl::~KVStoreBaseImpl() {
    mDhtNode.reset();

    FogKV::RTreeEngine::Close(mRTree.get());
    mRTree.reset();

    for (auto index = 0; index < _rqstPoolers.size(); index++) {
        delete _rqstPoolers.at(index);
    }

    if (_io_service)
        delete _io_service;
}

std::string KVStoreBaseImpl::getProperty(const std::string &name) {
    std::unique_lock<std::mutex> l(mLock);

    if (name == "fogkv.dht.id")
        return std::to_string(mDhtNode->getDhtId());
    if (name == "fogkv.listener.ip")
        return mDhtNode->getIp();
    if (name == "fogkv.listener.port")
        return std::to_string(mDhtNode->getPort());
    if (name == "fogkv.listener.dht_port")
        return std::to_string(mDhtNode->getDragonPort());
    if (name == "fogkv.pmem.path")
        return mOptions.PMEM.Path;
    if (name == "fogkv.pmem.size")
        return std::to_string(mOptions.PMEM.Size);
    if (name == "fogkv.KVEngine")
        return mRTree->Engine();
    if (name == "fogkv.dht.peers") {
        Json::Value peers;
        std::vector<FogKV::PureNode *> peerNodes;
        mDhtNode->getPeerList(peerNodes);

        int i = 0;
        for (auto peer : peerNodes) {
            peers[i]["id"] = std::to_string(peer->getDhtId());
            peers[i]["port"] = std::to_string(peer->getPort());
            peers[i]["ip"] = peer->getIp();
            peers[i]["dht_port"] = std::to_string(peer->getDragonPort());
        }

        Json::FastWriter writer;
        return writer.write(peers);
    }
    return "";
}

void KVStoreBaseImpl::init() {

    auto poolerCount = getOptions().Runtime.numOfPoolers;

    if (getOptions().Runtime.logFunc)
        gLog.setLogFunc(getOptions().Runtime.logFunc);

    _offloadReactor = new FogKV::OffloadReactor(
        POOLER_CPU_CORE_BASE + poolerCount + 1, mOptions.Runtime.spdkConfigFile,
        [&]() { mOptions.Runtime.shutdownFunc(); });

    while (_offloadReactor->state == ReactorState::INIT_INPROGRESS) {
        sleep(1);
    }
    if (_offloadReactor->state == ReactorState::READY) {
        _offloadEnabled = true;
        FOG_DEBUG("SPDK offload functionality enabled");
    }

    auto dhtPort =
        getOptions().Dht.Port ?: FogKV::utils::getFreePort(io_service(), 0);
    auto port = getOptions().Port ?: FogKV::utils::getFreePort(io_service(), 0);

    mDhtNode.reset(new FogKV::CChordAdapter(io_service(), dhtPort, port,
                                            getOptions().Dht.Id));

    mRTree.reset(FogKV::RTreeEngine::Open(mOptions.KVEngine, mOptions.PMEM.Path,
                                          mOptions.PMEM.Size));
    if (mRTree == nullptr)
        throw OperationFailedException(errno, ::pmemobj_errormsg());

    if (_offloadEnabled) {
        _offloadPooler =
            new FogKV::OffloadRqstPooler(mRTree, _offloadReactor->bdevContext,
                                         getOptions().Value.OffloadMaxSize);
        _offloadPooler->InitFreeList();
        _offloadReactor->RegisterPooler(_offloadPooler);
    }

    for (auto index = 0; index < poolerCount; index++) {
        auto rqstPooler =
            new FogKV::RqstPooler(mRTree, POOLER_CPU_CORE_BASE + index);
        if (_offloadEnabled)
            rqstPooler->offloadPooler = _offloadPooler;
        _rqstPoolers.push_back(rqstPooler);
    }

    FOG_DEBUG("KVStoreBaseImpl initialization completed");
}

asio::io_service &KVStoreBaseImpl::io_service() { return *_io_service; }

} // namespace FogKV
