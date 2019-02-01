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
    const string expectedVal = "fghi";
    const string expectedKey = "300";

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

    if (kvs->IsOffloaded(key)) {
        DAQDB_INFO << "Error: wrong value location";
        result = false;
    }

    DAQDB_INFO << format("Offload: [%1%]") % key.data();
    daqdb_offload(kvs, key);

    if (!kvs->IsOffloaded(key)) {
        DAQDB_INFO << "Error: wrong value location";
        result = false;
    }

    currVal = daqdb_get(kvs, key);
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

bool testAsyncOffloadOperations(KVStoreBase *kvs) {
    bool result = true;

    const string expectedVal = "oprstu";
    const string expectedKey = "400";

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

    key = strToKey(kvs, expectedKey);
    if (kvs->IsOffloaded(key)) {
        DAQDB_INFO << "Error: wrong value location";
        result = false;
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
                if (!value || expectedVal.compare(value) != 0) {
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

    daqdb_async_offload(
        kvs, key,
        [&](KVStoreBase *kvs, Status status, const char *key,
            const size_t keySize, const char *value, const size_t valueSize) {
            unique_lock<mutex> lck(mtx);
            if (status.ok()) {
                DAQDB_INFO << boost::format("Offload: [%1%]") % key;
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

    currVal = daqdb_get(kvs, key);
    if (!currVal.data() || expectedVal.compare(currVal.data()) != 0) {
        DAQDB_INFO << "Error: wrong value returned" << flush;
        result = false;
    }
    DAQDB_INFO << format("Get: [%1%] = %2%") % key.data() % currVal.data();

    daqdb_async_get(
        kvs, key,
        [&](KVStoreBase *kvs, Status status, const char *key, size_t keySize,
            const char *value, size_t valueSize) {
            unique_lock<mutex> lck(mtx);

            if (status.ok()) {
                DAQDB_INFO << boost::format("GetAsync: [%1%] = %2%") % key %
                                  value;
                if (!value || expectedVal.compare(value) != 0) {
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
