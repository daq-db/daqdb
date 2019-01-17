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

#include "base_operations.h"
#include "debug.h"

using namespace std;
using namespace DaqDB;

Value allocValue(KVStoreBase *kvs, const Key &key, const string &value) {
    Value result = kvs->Alloc(key, value.size() + 1);
    memcpy(result.data(), value.c_str(), value.size());
    result.data()[result.size() - 1] = '\0';
    return result;
}

Value allocAndFillValue(KVStoreBase *kvs, const Key &key,
                        const size_t valueSize) {
    Value result = kvs->Alloc(key, valueSize);
    for (int i = 0; i < valueSize; i++) {
        result.data()[i] = i % 256;
    }
    return result;
}

Key strToKey(KVStoreBase *kvs, const string &key) {
    Key keyBuff = kvs->AllocKey(KeyAttribute::NOT_BUFFERED);
    memset(keyBuff.data(), 0, keyBuff.size());
    memcpy(keyBuff.data(), key.c_str(), key.size());
    return keyBuff;
}

Value daqdb_get(KVStoreBase *kvs, const Key &key) {
    try {
        return kvs->Get(key);
    } catch (OperationFailedException &e) {
        if (e.status()() == KEY_NOT_FOUND) {
            BOOST_LOG_SEV(lg::get(), bt::info)
                << format("[%1%] not found") % key.data();
        } else {
            BOOST_LOG_SEV(lg::get(), bt::info)
                << "Error: cannot get element: " << e.status().to_string()
                << flush;
        }
    }
    return Value();
}

void daqdb_put(KVStoreBase *kvs, Key &&key, Value &val) {
    try {
        kvs->Put(move(key), move(val));
    } catch (OperationFailedException &e) {
        BOOST_LOG_SEV(lg::get(), bt::info)
            << "Error: cannot put element: " << e.status().to_string() << flush;
    }
}

void daqdb_update(KVStoreBase *kvs, Key &key, Value &val,
                  const UpdateOptions &options = UpdateOptions()) {
    try {
        kvs->Update(key, move(val), move(options));
    } catch (OperationFailedException &e) {
        BOOST_LOG_SEV(lg::get(), bt::info)
            << "Error: cannot update element: " << e.status().to_string()
            << flush;
    }
}

void daqdb_offload(KVStoreBase *kvs, Key &key) {
    try {
        UpdateOptions options(PrimaryKeyAttribute::LONG_TERM);
        kvs->Update(key, move(options));
    } catch (OperationFailedException &e) {
        BOOST_LOG_SEV(lg::get(), bt::info)
            << "Error: cannot update element: " << e.status().to_string()
            << flush;
    }
}

void daqdb_async_offload(KVStoreBase *kvs, Key &key,
                         KVStoreBase::KVStoreBaseCallback cb) {
    try {
        UpdateOptions options(PrimaryKeyAttribute::LONG_TERM);
        kvs->UpdateAsync(key, move(options), cb);
    } catch (OperationFailedException &e) {
        BOOST_LOG_SEV(lg::get(), bt::info)
            << "Error: cannot update element: " << e.status().to_string()
            << flush;
    }
}

bool daqdb_remove(KVStoreBase *kvs, Key &key) {

    try {
        kvs->Remove(key);
    } catch (OperationFailedException &e) {
        return false;
    }
    try {
        kvs->Get(key);
    } catch (OperationFailedException &e) {
        if (e.status()() == KEY_NOT_FOUND) {
            // success scenario: exception should thrown due non-existent key.
            return true;
        }
    }
    return false;
}

void daqdb_async_get(KVStoreBase *kvs, const Key &key,
                     KVStoreBase::KVStoreBaseCallback cb) {
    try {
        GetOptions options(PrimaryKeyAttribute::EMPTY);
        return kvs->GetAsync(key, cb, move(options));
    } catch (OperationFailedException &e) {
        BOOST_LOG_SEV(lg::get(), bt::info)
            << "Error: cannot get element: " << e.status().to_string() << flush;
    }
}

void daqdb_async_put(KVStoreBase *kvs, Key &&key, Value &val,
                     KVStoreBase::KVStoreBaseCallback cb) {
    try {
        PutOptions options(PrimaryKeyAttribute::EMPTY);
        kvs->PutAsync(move(key), move(val), cb, move(options));
    } catch (OperationFailedException &e) {
        BOOST_LOG_SEV(lg::get(), bt::info)
            << "Error: cannot put element: " << e.status().to_string() << flush;
    }
}

void daqdb_async_update(KVStoreBase *kvs, Key &key, Value &val,
                        KVStoreBase::KVStoreBaseCallback cb) {
    try {
        kvs->UpdateAsync(key, move(val), cb);
    } catch (OperationFailedException &e) {
        BOOST_LOG_SEV(lg::get(), bt::info)
            << "Error: cannot update element: " << e.status().to_string()
            << flush;
    }
}
