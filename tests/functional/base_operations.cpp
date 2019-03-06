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

Key allocKey(KVStoreBase *kvs, const uint64_t id) {
    Key keyBuff = kvs->AllocKey(KeyValAttribute::NOT_BUFFERED);
    FuncTestKey *fKeyPtr = reinterpret_cast<FuncTestKey *>(keyBuff.data());
    memset(fKeyPtr, 0, sizeof(FuncTestKey));
    memcpy(&fKeyPtr->eventId, &id, sizeof(fKeyPtr->eventId));
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
    std::stringstream ss;
    ss << "0x";
    for (int i = (sizeof(fKeyPtr->eventId) / sizeof(fKeyPtr->eventId[0])) - 1;
         i >= 0; i--)
        ss << std::hex << static_cast<unsigned int>(fKeyPtr->eventId[i]);

    return ss.str();
}

const std::string keyToStr(const char *key) {
    const FuncTestKey *fKeyPtr = reinterpret_cast<const FuncTestKey *>(key);
    std::stringstream ss;
    ss << "0x";
    for (int i = (sizeof(fKeyPtr->eventId) / sizeof(fKeyPtr->eventId[0])) - 1;
         i >= 0; i--)
        ss << std::hex << static_cast<unsigned int>(fKeyPtr->eventId[i]);

    return ss.str();
}

Value daqdb_get(KVStoreBase *kvs, const uint64_t keyId) {
    auto key = allocKey(kvs, keyId);
    try {
        auto result = kvs->Get(key);
        DAQDB_INFO << format("Get: [%1%] with size [%2%]") % keyId %
                          result.size();
        return result;
    } catch (OperationFailedException &e) {
        if (e.status()() == KEY_NOT_FOUND) {
            BOOST_LOG_SEV(lg::get(), bt::info)
                << format("[%1%] not found") % keyId;
        } else {
            BOOST_LOG_SEV(lg::get(), bt::info)
                << "Error: cannot get element: " << e.status().to_string()
                << flush;
        }
    }
    return Value();
}

void daqdb_put(KVStoreBase *kvs, const uint64_t keyId,
               const std::string &value) {
    auto key = allocKey(kvs, keyId);
    auto val = allocValue(kvs, keyId, value);
    DAQDB_INFO << format("Put: [%1%]") % keyId;

    try {
        kvs->Put(move(key), move(val));
    } catch (OperationFailedException &e) {
        BOOST_LOG_SEV(lg::get(), bt::info)
            << "Error: cannot put element: " << e.status().to_string() << flush;
    }
}

void daqdb_update(KVStoreBase *kvs, const uint64_t keyId, Value &val,
                  const UpdateOptions &options = UpdateOptions()) {
    auto key = allocKey(kvs, keyId);
    try {
        kvs->Update(key, move(val), move(options));
    } catch (OperationFailedException &e) {
        BOOST_LOG_SEV(lg::get(), bt::info)
            << "Error: cannot update element: " << e.status().to_string()
            << flush;
    }
}

void daqdb_offload(KVStoreBase *kvs, const uint64_t keyId) {
    auto key = allocKey(kvs, keyId);
    DAQDB_INFO << format("Offload: [%1%]") % keyId;
    try {
        UpdateOptions options(PrimaryKeyAttribute::LONG_TERM);
        kvs->Update(key, move(options));
    } catch (OperationFailedException &e) {
        BOOST_LOG_SEV(lg::get(), bt::info)
            << "Error: cannot update element: " << e.status().to_string()
            << flush;
    }
}

void daqdb_async_offload(KVStoreBase *kvs, const uint64_t keyId,
                         KVStoreBase::KVStoreBaseCallback cb) {
    auto key = allocKey(kvs, keyId);
    try {
        UpdateOptions options(PrimaryKeyAttribute::LONG_TERM);
        kvs->UpdateAsync(key, move(options), cb);
    } catch (OperationFailedException &e) {
        BOOST_LOG_SEV(lg::get(), bt::info)
            << "Error: cannot update element: " << e.status().to_string()
            << flush;
    }
}

bool daqdb_remove(KVStoreBase *kvs, const uint64_t keyId) {
    auto key = allocKey(kvs, keyId);
    try {
        DAQDB_INFO << format("Remove: [%1%]") % keyId;
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

void daqdb_async_get(KVStoreBase *kvs, const uint64_t keyId,
                     KVStoreBase::KVStoreBaseCallback cb) {
    auto key = allocKey(kvs, keyId);
    try {
        GetOptions options(PrimaryKeyAttribute::EMPTY);
        return kvs->GetAsync(key, cb, move(options));
    } catch (OperationFailedException &e) {
        BOOST_LOG_SEV(lg::get(), bt::info)
            << "Error: cannot get element: " << e.status().to_string() << flush;
    }
}

void daqdb_async_put(KVStoreBase *kvs, const uint64_t keyId,
                     const std::string &value,
                     KVStoreBase::KVStoreBaseCallback cb) {
    auto key = allocKey(kvs, keyId);
    auto val = allocValue(kvs, keyId, value);
    try {
        PutOptions options(PrimaryKeyAttribute::EMPTY);
        kvs->PutAsync(move(key), move(val), cb, move(options));
    } catch (OperationFailedException &e) {
        BOOST_LOG_SEV(lg::get(), bt::info)
            << "Error: cannot put element: " << e.status().to_string() << flush;
    }
}

void daqdb_async_update(KVStoreBase *kvs, const uint64_t keyId, Value &val,
                        KVStoreBase::KVStoreBaseCallback cb) {
    auto key = allocKey(kvs, keyId);
    try {
        kvs->UpdateAsync(key, move(val), cb);
    } catch (OperationFailedException &e) {
        BOOST_LOG_SEV(lg::get(), bt::info)
            << "Error: cannot update element: " << e.status().to_string()
            << flush;
    }
}
