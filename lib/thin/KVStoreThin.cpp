/**
 * Copyright 2018 Intel Corporation.
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

#include "KVStoreThin.h"
#include <DhtUtils.h>
#include <Logger.h>

namespace DaqDB {

KVStoreBase *KVStoreThin::Open(const Options &options) {
    KVStoreThin *kvs = new KVStoreThin(options);
    kvs->init();
    return kvs;
}

KVStoreThin::KVStoreThin(const Options &options) : env(options, this) {}

KVStoreThin::~KVStoreThin() {}

void KVStoreThin::init() {
    if (getOptions().runtime.logFunc)
        gLog.setLogFunc(getOptions().runtime.logFunc);

    auto dhtPort =
        getOptions().dht.port ?: DaqDB::utils::getFreePort(env.ioService(), 0);
    _dht.reset(new DaqDB::ZhtNode(env.ioService(), &env, dhtPort));
    while (_dht->state == DhtServerState::DHT_INIT) {
        sleep(1);
    }
    if (_dht->state == DhtServerState::DHT_READY) {
        DAQ_DEBUG("DHT server started successfully");
    } else {
        DAQ_DEBUG("Can not start DHT server");
    }

    DAQ_DEBUG("KVStoreThin initialization completed");
}

const Options &KVStoreThin::getOptions() { return env.getOptions(); }

size_t KVStoreThin::KeySize() { return env.keySize(); }

void KVStoreThin::LogMsg(std::string msg) {
    if (getOptions().runtime.logFunc) {
        getOptions().runtime.logFunc(msg);
    }
}

Value KVStoreThin::Get(const Key &key, const GetOptions &options) {
    return _dht->Get(key);
}

void KVStoreThin::Put(Key &&key, Value &&val, const PutOptions &options) {
    return _dht->Put(key, val);
}

void KVStoreThin::PutAsync(Key &&key, Value &&value, KVStoreBaseCallback cb,
                           const PutOptions &options) {
    throw FUNC_NOT_IMPLEMENTED;
}

void KVStoreThin::GetAsync(const Key &key, KVStoreBaseCallback cb,
                           const GetOptions &options) {
    throw FUNC_NOT_IMPLEMENTED;
}

Key KVStoreThin::GetAny(const GetOptions &options) {
    throw FUNC_NOT_IMPLEMENTED;
}

void KVStoreThin::GetAnyAsync(KVStoreBaseGetAnyCallback cb,
                              const GetOptions &options) {
    throw FUNC_NOT_IMPLEMENTED;
}

void KVStoreThin::Update(const Key &key, Value &&val,
                         const UpdateOptions &options) {
    throw FUNC_NOT_IMPLEMENTED;
}

void KVStoreThin::Update(const Key &key, const UpdateOptions &options) {
    throw FUNC_NOT_IMPLEMENTED;
}

void KVStoreThin::UpdateAsync(const Key &key, Value &&value,
                              KVStoreBaseCallback cb,
                              const UpdateOptions &options) {
    throw FUNC_NOT_IMPLEMENTED;
}

void KVStoreThin::UpdateAsync(const Key &key, const UpdateOptions &options,
                              KVStoreBaseCallback cb) {
    throw FUNC_NOT_IMPLEMENTED;
}

std::vector<KVPair> KVStoreThin::GetRange(const Key &beg, const Key &end,
                                          const GetOptions &options) {
    throw FUNC_NOT_IMPLEMENTED;
}

void KVStoreThin::GetRangeAsync(const Key &beg, const Key &end,
                                KVStoreBaseRangeCallback cb,
                                const GetOptions &options) {
    throw FUNC_NOT_IMPLEMENTED;
}

void KVStoreThin::Remove(const Key &key) { throw FUNC_NOT_IMPLEMENTED; }

void KVStoreThin::RemoveRange(const Key &beg, const Key &end) {
    throw FUNC_NOT_IMPLEMENTED;
}

Value KVStoreThin::Alloc(const Key &key, size_t size,
                         const AllocOptions &options) {
    if (size == 0)
        throw OperationFailedException(ALLOCATION_ERROR);

    return Value(new char[size], size);
}

void KVStoreThin::Free(Value &&value) { delete[] value.data(); }

Key KVStoreThin::AllocKey(const AllocOptions &options) {
    return Key(new char[env.keySize()], env.keySize());
}

void KVStoreThin::Realloc(Value &value, size_t size,
                          const AllocOptions &options) {
    throw FUNC_NOT_IMPLEMENTED;
}

void KVStoreThin::ChangeOptions(Value &value, const AllocOptions &options) {
    throw FUNC_NOT_IMPLEMENTED;
}

void KVStoreThin::Free(Key &&key) { delete[] key.data(); }

void KVStoreThin::ChangeOptions(Key &key, const AllocOptions &options) {
    throw FUNC_NOT_IMPLEMENTED;
}

bool KVStoreThin::IsOffloaded(Key &key) { throw FUNC_NOT_IMPLEMENTED; }

std::string KVStoreThin::getProperty(const std::string &name) {
    if (name == "daqdb.dht.id")
        return std::to_string(_dht->getDhtId());
    if (name == "daqdb.dht.neighbours")
        return _dht->printNeighbors();
    if (name == "daqdb.dht.status")
        return _dht->printStatus();
    if (name == "daqdb.dht.ip")
        return _dht->getIp();
    if (name == "daqdb.dht.port")
        return std::to_string(_dht->getPort());

    return "";
}

} // namespace DaqDB
