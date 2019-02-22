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

#include "base_operations.h"

#include "config.h"
#include "debug.h"

using namespace std;
using namespace DaqDB;

Value allocValue(KVStoreBase *kvs, const uint64_t keyId, const string &value) {
    auto key = allocKey(kvs, keyId);
    Value result = kvs->Alloc(key, value.size());
    memcpy(result.data(), value.c_str(), value.size());
    return result;
}

Value allocValue(KVStoreBase *kvs, const DaqDB::Key &key, const string &value) {
    Value result = kvs->Alloc(key, value.size());
    memcpy(result.data(), value.c_str(), value.size());
    return result;
}

Key allocKey(KVStoreBase *kvs, const uint64_t id) {
    Key keyBuff = kvs->AllocKey();
    FuncTestKey *fKeyPtr = reinterpret_cast<FuncTestKey *>(keyBuff.data());
    fKeyPtr->runId = 0;
    fKeyPtr->subdetectorId = 0;
    fKeyPtr->eventId = id;
    return keyBuff;
}

const std::string generateValueStr(const size_t valueSize) {
    string result(valueSize, 0);
    generate_n(result.begin(), valueSize,
               [n = 0]() mutable { return n++ % 256; });
    return result;
}

bool checkValue(const string &expectedValue, Value *value) {
    if (!value || !value->data()) {
        DAQDB_INFO << "Error: value is empty" << flush;
        return false;
    }
    if (memcmp(expectedValue.data(), value->data(), expectedValue.size())) {
        DAQDB_INFO << "Error: wrong value returned" << flush;
        return false;
    }
    return true;
}

const std::string keyToStr(Key &key) {
    FuncTestKey *fKeyPtr = reinterpret_cast<FuncTestKey *>(key.data());
    return std::to_string(fKeyPtr->eventId);
}

const std::string keyToStr(const char *key) {
    const FuncTestKey *fKeyPtr = reinterpret_cast<const FuncTestKey *>(key);
    return std::to_string(fKeyPtr->eventId);
}

Value remote_get(KVStoreBase *kvs, const uint64_t keyId) {
    auto key = allocKey(kvs, keyId);
    try {
        auto result = kvs->Get(key);
        kvs->Free(move(key));
        return result;
    }
    catch (OperationFailedException &e) {
        if (e.status()() == KEY_NOT_FOUND) {
            DAQDB_INFO << format("[%1%] not found") % key.data();
        } else {
            DAQDB_INFO << "Error: cannot get element: "
                       << e.status().to_string() << flush;
        }
    }
    kvs->Free(move(key));
    return Value();
}

void remote_put(KVStoreBase *kvs, const uint64_t keyId,
                const std::string &value) {
    auto key = allocKey(kvs, keyId);
    auto val = allocValue(kvs, key, value);
    try {
        kvs->Put(move(key), move(val));
    }
    catch (OperationFailedException &e) {
        DAQDB_INFO << "Error: cannot put element: " << e.status().to_string()
                   << flush;
    }
    kvs->Free(move(key));
}

bool remote_remove(KVStoreBase *kvs, const uint64_t keyId) {
    bool result = false;
    auto key = allocKey(kvs, keyId);
    try {
        kvs->Remove(key);
    }
    catch (OperationFailedException &e) {
        DAQDB_INFO << "Error: cannot remove element" << flush;
        return false;
    }
    try {
        kvs->Get(key);
    }
    catch (OperationFailedException &e) {
        // success scenario: exception should thrown due non-existent key.
        result = (e.status().getStatusCode() == StatusCode::KEY_NOT_FOUND);
        if (!result) {
            DAQDB_INFO << "Error: cannot remove element: status code ["
                       << e.status().getStatusCode() << "]" << flush;
        }
    }
    kvs->Free(move(key));
    return result;
}
