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

#include <chrono>
#include <condition_variable>

#include "tests.h"
#include <base_operations.h>
#include <debug.h>

using namespace std;
using namespace DaqDB;
using namespace chrono_literals;

bool testSyncOperations(KVStoreBase *kvs) {
    bool result = true;
    const string val = "abcd";
    const uint64_t keyId = 100;

    daqdb_put(kvs, keyId, val);
    auto resultVal = daqdb_get(kvs, keyId);
    if (!checkValue(val, &resultVal)) {
        result = false;
    }

    auto removeResult = daqdb_remove(kvs, keyId);
    if (!removeResult) {
        result = false;
        DAQDB_INFO << format("Error: Cannot remove a key [%1%]") % keyId;
    }

    return result;
}

bool testAsyncOperations(KVStoreBase *kvs) {
    bool result = true;
    const string val = "wxyz";
    const uint64_t keyId = 200;

    mutex mtx;
    condition_variable cv;
    bool ready = false;

    daqdb_async_put(
        kvs, keyId, val,
        [&](KVStoreBase *kvs, Status status, const char *argKey,
            const size_t keySize, const char *value, const size_t valueSize) {
            unique_lock<mutex> lck(mtx);
            if (status.ok()) {
                DAQDB_INFO << boost::format("PutAsync: [%1%]") %
                                  keyToStr(argKey);
            } else {
                DAQDB_INFO << boost::format("Error: cannot put element: %1%") %
                                  status.to_string();
                result = false;
            }
            ready = true;
            cv.notify_all();
        });

    // wait for completion
    {
        unique_lock<mutex> lk(mtx);
        cv.wait_for(lk, 1s, [&ready] { return ready; });
        ready = false;
    }

    Value resultVal = daqdb_get(kvs, keyId);
    if (!checkValue(val, &resultVal)) {
        result = false;
    }

    daqdb_async_get(
        kvs, keyId,
        [&](KVStoreBase *kvs, Status status, const char *argKey, size_t keySize,
            const char *value, size_t valueSize) {
            unique_lock<mutex> lck(mtx);

            if (status.ok()) {
                DAQDB_INFO << boost::format("GetAsync: [%1%]") %
                                  keyToStr(argKey);

                if (!value || memcmp(val.data(), value, val.size())) {
                    DAQDB_INFO << "Error: wrong value returned" << flush;
                    result = false;
                }
            } else {
                DAQDB_INFO << boost::format("Error: cannot get element: %1%") %
                                  status.to_string();
                result = false;
            }

            ready = true;
            cv.notify_all();
        });

    // wait for completion
    {
        unique_lock<mutex> lk(mtx);
        cv.wait_for(lk, 1s, [&ready] { return ready; });
        ready = false;
    }

    auto removeResult = daqdb_remove(kvs, keyId);
    if (!removeResult) {
        result = false;
        DAQDB_INFO << format("Error: Cannot remove a key [%1%]") % keyId;
    }

    return result;
}

bool testMultiplePuts(KVStoreBase *kvs) {
    bool result = true;
    const uint64_t keyId = 300;
    const uint64_t keyId2 = 301;
    const string val = "abcd";
    const string val2 = "efg";

    daqdb_put(kvs, keyId, val);
    daqdb_put(kvs, keyId2, val2);

    auto resultVal = daqdb_get(kvs, keyId);
    if (!checkValue(val, &resultVal)) {
        result = false;
    }

    resultVal = daqdb_get(kvs, keyId2);
    if (!checkValue(val2, &resultVal)) {
        result = false;
    }

    auto removeResult = daqdb_remove(kvs, keyId);
    if (!removeResult) {
        result = false;
        DAQDB_INFO << format("Error: Cannot remove a key [%1%]") % keyId;
    }

    removeResult = daqdb_remove(kvs, keyId2);
    if (!removeResult) {
        result = false;
        DAQDB_INFO << format("Error: Cannot remove a key [%1%]") % keyId2;
    }

    return result;
}
