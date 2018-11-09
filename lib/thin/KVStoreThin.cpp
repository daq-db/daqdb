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

#include <Logger.h>

namespace DaqDB {

#ifdef THIN_LIB
KVStoreBase *KVStoreBase::Open(const Options &options) {
    return KVStoreThin::Open(options);
}
#endif

KVStoreBase *KVStoreThin::Open(const Options &options) {
    KVStoreThin *kvs = new KVStoreThin(options);
    return kvs;
}

KVStoreThin::KVStoreThin(const Options &options) : env(options, this) {}

KVStoreThin::~KVStoreThin() {}

const Options &KVStoreThin::getOptions() { return env.getOptions(); }

size_t KVStoreThin::KeySize() { return env.keySize(); }

void KVStoreThin::LogMsg(std::string msg) {
    if (getOptions().Runtime.logFunc) {
        getOptions().Runtime.logFunc(msg);
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
    throw FUNC_NOT_IMPLEMENTED;
}

void KVStoreThin::Free(Value &&value) { throw FUNC_NOT_IMPLEMENTED; }

Key KVStoreThin::AllocKey(const AllocOptions &options) {
    throw FUNC_NOT_IMPLEMENTED;
}

void KVStoreThin::Realloc(Value &value, size_t size,
                          const AllocOptions &options) {
    throw FUNC_NOT_IMPLEMENTED;
}

void KVStoreThin::ChangeOptions(Value &value, const AllocOptions &options) {
    throw FUNC_NOT_IMPLEMENTED;
}

void KVStoreThin::Free(Key &&key) { throw FUNC_NOT_IMPLEMENTED; }

void KVStoreThin::ChangeOptions(Key &key, const AllocOptions &options) {
    throw FUNC_NOT_IMPLEMENTED;
}

bool KVStoreThin::IsOffloaded(Key &key) { throw FUNC_NOT_IMPLEMENTED; }

std::string KVStoreThin::getProperty(const std::string &name) {
    throw FUNC_NOT_IMPLEMENTED;
}

} // namespace DaqDB
