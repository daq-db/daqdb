/**
 * Copyright 2018-2019 Intel Corporation.
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

const size_t DEFAULT_KEY_SIZE = 16;

KVStoreBase *KVStoreThin::Open(const DaqDB::Options &options) {
    KVStoreThin *kvs = new KVStoreThin(options);
    kvs->init();
    return kvs;
}

KVStoreThin::KVStoreThin(const DaqDB::Options &options) : _options(options) {
    for (size_t i = 0; i < options.key.nfields(); i++)
        _keySize += options.key.field(i).size;
    if (_keySize)
        _keySize = DEFAULT_KEY_SIZE;
}

KVStoreThin::~KVStoreThin() {}

void KVStoreThin::init() {
    if (getOptions().runtime.logFunc)
        gLog.setLogFunc(getOptions().runtime.logFunc);

    _spDht.reset(new DhtCore(getOptions().dht));
    _spDht->initClient();
    if (_spDht->getClient()->state == DhtClientState::DHT_CLIENT_READY) {
        DAQ_DEBUG("DHT client started successfully");
    } else {
        DAQ_DEBUG("Can not start DHT client");
    }

    DAQ_DEBUG("KVStoreThin initialization completed");
}

size_t KVStoreThin::KeySize() { return _keySize; }

const Options &KVStoreThin::getOptions() { return _options; }

void KVStoreThin::LogMsg(std::string msg) {
    if (getOptions().runtime.logFunc) {
        getOptions().runtime.logFunc(msg);
    }
}

Value KVStoreThin::Get(const Key &key, const GetOptions &options) {
    return dhtClient()->get(key);
}

void KVStoreThin::Put(Key &&key, Value &&val, const PutOptions &options) {
    try {
        dhtClient()->put(key, val);
    } catch (...) {
        dhtClient()->free(std::move(key));
        throw;
    }
    dhtClient()->free(std::move(key));
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

void KVStoreThin::Remove(const Key &key) { dhtClient()->remove(key); }

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
    if (options.attr & KeyAttribute::DHT_BUFFERED) {
        return dhtClient()->allocKey(KeySize());
    } else {
        return Key(new char[KeySize()], KeySize());
    }
}

void KVStoreThin::Realloc(Value &value, size_t size,
                          const AllocOptions &options) {
    throw FUNC_NOT_SUPPORTED;
}

void KVStoreThin::ChangeOptions(Value &value, const AllocOptions &options) {
    throw FUNC_NOT_SUPPORTED;
}

void KVStoreThin::Free(Key &&key) {
    if (key.isDhtBuffered()) {
        dhtClient()->free(std::move(key));
    } else {
        delete[] key.data();
    }
}

void KVStoreThin::ChangeOptions(Key &key, const AllocOptions &options) {
    throw FUNC_NOT_SUPPORTED;
}

bool KVStoreThin::IsOffloaded(Key &key) { throw FUNC_NOT_SUPPORTED; }

std::string KVStoreThin::getProperty(const std::string &name) { return ""; }

} // namespace DaqDB
