/**
 * Copyright 2017, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of the copyright holder nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "KVStoreBaseImpl.h"
#include "../debug/Logger.h"
#include "../dht/DhtUtils.h"
#include "common.h"
#include <FogKV/Types.h>
#include <future>
#include <json/json.h>

/** @TODO jradtke: should be taken from configuration file */
#define POOLER_CPU_CORE_BASE 1

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
    StatusCode rc = mRTree->Put(key.data(), val.data());
    // Free(std::move(val)); /** @TODO jschmieg: free value if needed */
    if (rc != StatusCode::Ok)
        throw OperationFailedException(EINVAL);
}

void KVStoreBaseImpl::PutAsync(Key &&key, Value &&value, KVStoreBaseCallback cb,
                               const PutOptions &options) {
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
    StatusCode rc = mRTree->Get(key.data(), &pVal, &size);
    if (rc != StatusCode::Ok || !pVal) {
        if (rc == StatusCode::KeyNotFound)
            throw OperationFailedException(KeyNotFound);

        throw OperationFailedException(EINVAL);
    }

    Value value(new char[size], size);
    std::memcpy(value.data(), pVal, size);
    return value;
}

void KVStoreBaseImpl::GetAsync(const Key &key, KVStoreBaseCallback cb,
                               const GetOptions &options) {
    thread_local int poolerId = 0;

    poolerId = (options.roundRobin()) ? ((poolerId + 1) % _rqstPoolers.size())
                                      : options.poolerId();

    if (!key.data()) {
        throw OperationFailedException(EINVAL);
    }
    try {
        if (!_rqstPoolers.at(poolerId)->EnqueueMsg(new RqstMsg(
                RqstOperation::GET, key.data(), key.size(), nullptr, 0, cb))) {
            throw QueueFullException();
        }
    } catch (OperationFailedException &e) {
        Value val;
        cb(this, e.status(), key.data(), key.size(), val.data(), val.size());
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

    throw FUNC_NOT_IMPLEMENTED;
}

void KVStoreBaseImpl::Update(const Key &key, const UpdateOptions &options) {
    throw FUNC_NOT_IMPLEMENTED;
}

void KVStoreBaseImpl::UpdateAsync(const Key &key, Value &&value,
                                  KVStoreBaseCallback cb,
                                  const UpdateOptions &options) {
    throw FUNC_NOT_IMPLEMENTED;
}

void KVStoreBaseImpl::UpdateAsync(const Key &key, const UpdateOptions &options,
                                  KVStoreBaseCallback cb) {
    throw FUNC_NOT_IMPLEMENTED;
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

    StatusCode rc = mRTree->Remove(key.data());
    if (rc != StatusCode::Ok) {
        if (rc == StatusCode::KeyNotFound)
            throw OperationFailedException(KeyNotFound);

        throw OperationFailedException(EINVAL);
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
    : mOptions(options), mKeySize(0) {
    if (mOptions.Runtime.io_service() == nullptr)
        m_io_service = new asio::io_service();

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

    if (m_io_service)
        delete m_io_service;
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

    if (getOptions().Runtime.logFunc)
        gLog.setLogFunc(getOptions().Runtime.logFunc);

    auto dhtPort =
        getOptions().Dht.Port ?: FogKV::utils::getFreePort(io_service(), 0);
    auto port = getOptions().Port ?: FogKV::utils::getFreePort(io_service(), 0);

    mDhtNode.reset(new FogKV::CChordAdapter(io_service(), dhtPort, port,
                                            getOptions().Dht.Id));

    mRTree.reset(FogKV::RTreeEngine::Open(mOptions.KVEngine, mOptions.PMEM.Path,
                                          mOptions.PMEM.Size));
    if (mRTree == nullptr)
        throw OperationFailedException(errno, ::pmemobj_errormsg());

    struct spdk_env_opts opts;
    // @TODO jradtke: SPDK init messages should be hidden.
    spdk_env_opts_init(&opts);
    opts.name = "FogKV";
    opts.shm_id = 0;
    if (spdk_env_init(&opts) == 0) {
        auto poolerCount = getOptions().Runtime.numOfPoolers();
        for (auto index = 0; index < poolerCount; index++) {
            _rqstPoolers.push_back(
                new FogKV::RqstPooler(mRTree, POOLER_CPU_CORE_BASE + index));
        }
    }

    FOG_DEBUG("KVStoreBaseImpl initialization completed");
}

asio::io_service &KVStoreBaseImpl::io_service() {
    if (mOptions.Runtime.io_service())
        return *mOptions.Runtime.io_service();
    return *m_io_service;
}

} // namespace FogKV
