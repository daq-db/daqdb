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

using namespace std::chrono_literals;
using namespace std;
using namespace DaqDB;

bool testSyncOffloadOperations(KVStoreBase *kvs) {
    bool result = true;
    const string val = "fghi";
    const uint64_t keyId = 300;

    daqdb_put(kvs, keyId, val);

    auto resultVal = daqdb_get(kvs, keyId);
    if (!checkValue(val, &resultVal)) {
        result = false;
    }

    auto key = allocKey(kvs, keyId);
    if (kvs->IsOffloaded(key)) {
        DAQDB_INFO << "Error: wrong value location";
        result = false;
    }

    daqdb_offload(kvs, keyId);

    if (!kvs->IsOffloaded(key)) {
        DAQDB_INFO << "Error: wrong value location";
        result = false;
    }

    resultVal = daqdb_get(kvs, keyId);
    if (!checkValue(val, &resultVal)) {
        result = false;
    }

    auto removeResult = daqdb_remove(kvs, keyId);
    if (!removeResult) {
        result = false;
        DAQDB_INFO << format("Error: Cannot remove a key [%1%]") % key.data();
    }

    return result;
}

bool testAsyncOffloadOperations(KVStoreBase *kvs) {
    bool result = true;

    const string val = "oprstu";
    const uint64_t keyId = 400;

    mutex mtx;
    condition_variable cv;
    bool ready = false;

    daqdb_async_put(kvs, keyId, val,
                    [&](KVStoreBase *kvs, Status status, const char *argKey,
                        const size_t keySize, const char *value,
                        const size_t valueSize) {
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

    auto key = allocKey(kvs, keyId);
    if (kvs->IsOffloaded(key)) {
        DAQDB_INFO << "Error: wrong value location";
        result = false;
    }

    auto resultVal = daqdb_get(kvs, keyId);
    if (!checkValue(val, &resultVal)) {
        result = false;
    }

    daqdb_async_get(kvs, keyId,
                    [&](KVStoreBase *kvs, Status status, const char *argKey,
                        size_t keySize, const char *value, size_t valueSize) {
            unique_lock<mutex> lck(mtx);

            if (status.ok()) {
                DAQDB_INFO << boost::format("GetAsync: [%1%] = %2%") %
                                  keyToStr(argKey) % value;
                if (!value || val.compare(value) != 0) {
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

    daqdb_async_offload(kvs, keyId,
                        [&](KVStoreBase *kvs, Status status, const char *argKey,
                            const size_t keySize, const char *value,
                            const size_t valueSize) {
            unique_lock<mutex> lck(mtx);
            if (status.ok()) {
                DAQDB_INFO << boost::format("Offload: [%1%]") %
                                  keyToStr(argKey);
            } else {
                DAQDB_INFO << boost::format("Offload Error: %1%") %
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

    if (!kvs->IsOffloaded(key)) {
        DAQDB_INFO << "Error: wrong value location";
        result = false;
    }

    resultVal = daqdb_get(kvs, keyId);
    if (!checkValue(val, &resultVal)) {
        result = false;
    }

    daqdb_async_get(kvs, keyId,
                    [&](KVStoreBase *kvs, Status status, const char *argKey,
                        size_t keySize, const char *value, size_t valueSize) {
            unique_lock<mutex> lck(mtx);

            if (status.ok()) {
                DAQDB_INFO << boost::format("GetAsync: [%1%] = %2%") %
                                  keyToStr(argKey) % value;
                if (!value || val.compare(value) != 0) {
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
