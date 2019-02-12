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

#include "KVStoreThin.h"
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
    _spDht->initNexus();
    _spDht->initClient();

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
        dhtClient()->free(key, std::move(val));
        dhtClient()->free(std::move(key));
        throw;
    }
    dhtClient()->free(key, std::move(val));
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

Key KVStoreThin::GetAny(const AllocOptions &allocOptions,
                        const GetOptions &options) {
    return dhtClient()->getAny();
}

void KVStoreThin::GetAnyAsync(KVStoreBaseGetAnyCallback cb,
                              const AllocOptions &allocOptions,
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
    if (options.attr & KeyValAttribute::KVS_BUFFERED) {
        return dhtClient()->alloc(key, size);
    } else {
        return Value(new char[size], size);
    }
}

void KVStoreThin::Free(const Key &key, Value &&value) {
    if (value.isKvsBuffered())
        return dhtClient()->free(key, std::move(value));
    else
        delete[] value.data();
}

Key KVStoreThin::AllocKey(const AllocOptions &options) {
    if (options.attr & KeyValAttribute::KVS_BUFFERED) {
        return dhtClient()->allocKey(KeySize());
    } else {
        return Key(new char[KeySize()], KeySize());
    }
}

void KVStoreThin::Realloc(const Key &key, Value &value, size_t size,
                          const AllocOptions &options) {
    throw FUNC_NOT_SUPPORTED;
}

void KVStoreThin::ChangeOptions(Value &value, const AllocOptions &options) {
    throw FUNC_NOT_SUPPORTED;
}

void KVStoreThin::Free(Key &&key) {
    if (key.isKvsBuffered()) {
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
