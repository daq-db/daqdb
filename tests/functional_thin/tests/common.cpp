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

#include "common.h"
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
    Key keyBuff = kvs->AllocKey();
    memset(keyBuff.data(), 0, keyBuff.size());
    memcpy(keyBuff.data(), key.c_str(), key.size());
    return keyBuff;
}

Key strToKey(KVStoreBase *kvs, const string &key, KeyAttribute attr) {
    Key keyBuff = kvs->AllocKey(attr);
    memset(keyBuff.data(), 0, keyBuff.size());
    memcpy(keyBuff.data(), key.c_str(), key.size());
    return keyBuff;
}

Value remote_get(KVStoreBase *kvs, const Key &key) {
    try {
        return kvs->Get(key);
    } catch (OperationFailedException &e) {
        if (e.status()() == KEY_NOT_FOUND) {
            LOG_INFO << format("[%1%] not found") % key.data();
        } else {
            LOG_INFO << "Error: cannot get element: " << e.status().to_string()
                     << flush;
        }
    }
    return Value();
}

void remote_put(KVStoreBase *kvs, Key &&key, Value &val) {
    try {
        kvs->Put(move(key), move(val));
    } catch (OperationFailedException &e) {
        LOG_INFO << "Error: cannot put element: " << e.status().to_string()
                 << flush;
    }
}

bool remote_remove(KVStoreBase *kvs, Key &key) {
    bool result = false;
    try {
        kvs->Remove(key);
    } catch (OperationFailedException &e) {
        LOG_INFO << "Error: cannot remove element" << flush;
        return false;
    }
    try {
        kvs->Get(key);
    } catch (OperationFailedException &e) {
        // success scenario: exception should thrown due non-existent key.
        result = (e.status().getStatusCode() == StatusCode::KEY_NOT_FOUND);
        if (!result) {
            LOG_INFO << "Error: cannot remove element: status code ["
                     << e.status().getStatusCode() << "]" << flush;
        }
    }
    return result;
}
