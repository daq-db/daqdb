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
    const string expectedVal = "abcd";
    const string expectedKey = "100";

    auto key = strToKey(kvs, expectedKey);
    auto val = allocValue(kvs, key, expectedVal);

    DAQDB_INFO << format("Put: [%1%] = %2%") % key.data() % val.data();
    daqdb_put(kvs, move(key), val);

    key = strToKey(kvs, expectedKey);
    auto currVal = daqdb_get(kvs, key);
    DAQDB_INFO << format("Get: [%1%] = %2%") % key.data() % currVal.data();

    if (!currVal.data() || expectedVal.compare(currVal.data()) != 0) {
        DAQDB_INFO << "Error: wrong value returned" << flush;
        result = false;
    }

    auto removeResult = daqdb_remove(kvs, key);
    DAQDB_INFO << format("Remove: [%1%]") % key.data();
    if (!removeResult) {
        result = false;
        DAQDB_INFO << format("Error: Cannot remove a key [%1%]") % key.data();
    }

    return result;
}

bool testAsyncOperations(KVStoreBase *kvs) {
    bool result = true;
    const string expectedVal = "wxyz";
    const string expectedKey = "200";

    auto key = strToKey(kvs, expectedKey);
    auto val = allocValue(kvs, key, expectedVal);

    mutex mtx;
    condition_variable cv;
    bool ready = false;

    daqdb_async_put(
        kvs, move(key), val,
        [&](KVStoreBase *kvs, Status status, const char *key,
            const size_t keySize, const char *value, const size_t valueSize) {
            unique_lock<mutex> lck(mtx);
            if (status.ok()) {
                DAQDB_INFO << boost::format("PutAsync: [%1%]") % key;
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

    auto currVal = daqdb_get(kvs, key);
    DAQDB_INFO << format("Get: [%1%] = %2%") % key.data() % currVal.data();
    if (!currVal.data() || expectedVal.compare(currVal.data()) != 0) {
        DAQDB_INFO << "Error: wrong value returned" << flush;
        result = false;
    }

    daqdb_async_get(
        kvs, key,
        [&](KVStoreBase *kvs, Status status, const char *key, size_t keySize,
            const char *value, size_t valueSize) {
            unique_lock<mutex> lck(mtx);

            if (status.ok()) {
                DAQDB_INFO << boost::format("GetAsync: [%1%] = %2%") % key %
                                  value;
                if (!currVal.data() || expectedVal.compare(value) != 0) {
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

    auto removeResult = daqdb_remove(kvs, key);
    DAQDB_INFO << format("Remove: [%1%]") % key.data();
    if (!removeResult) {
        result = false;
        DAQDB_INFO << format("Error: Cannot remove a key [%1%]") % key.data();
    }

    return result;
}
