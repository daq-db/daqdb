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

Key strToKey(KVStoreBase *kvs, const string &key, KeyValAttribute attr) {
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
            DAQDB_INFO << format("[%1%] not found") % key.data();
        } else {
            DAQDB_INFO << "Error: cannot get element: "
                       << e.status().to_string() << flush;
        }
    }
    return Value();
}

void remote_put(KVStoreBase *kvs, Key &&key, Value &val) {
    try {
        kvs->Put(move(key), move(val));
    } catch (OperationFailedException &e) {
        DAQDB_INFO << "Error: cannot put element: " << e.status().to_string()
                   << flush;
    }
}

bool remote_remove(KVStoreBase *kvs, Key &key) {
    bool result = false;
    try {
        kvs->Remove(key);
    } catch (OperationFailedException &e) {
        DAQDB_INFO << "Error: cannot remove element" << flush;
        return false;
    }
    try {
        kvs->Get(key);
    } catch (OperationFailedException &e) {
        // success scenario: exception should thrown due non-existent key.
        result = (e.status().getStatusCode() == StatusCode::KEY_NOT_FOUND);
        if (!result) {
            DAQDB_INFO << "Error: cannot remove element: status code ["
                       << e.status().getStatusCode() << "]" << flush;
        }
    }
    return result;
}
