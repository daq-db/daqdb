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

bool use_case_sync_base(DaqDB::KVStoreBase *kvs) {
    bool result = true;
    const std::string expectedVal = "abcd";
    const std::string expectedKey = "100";

    auto key = strToKey(kvs, expectedKey);
    auto val = allocValue(kvs, key, expectedVal);

    daqdb_put(kvs, key, val);
    LOG_INFO << format("Put: [%1%] = %2%") % key.data() % val.data();
    auto currVal = daqdb_get(kvs, key);
    LOG_INFO << format("Get: [%1%] = %2%") % key.data() % currVal.data();

    if (expectedVal.compare(currVal.data()) != 0) {
        LOG_INFO << "Error: wrong value returned" << std::flush;
        result = false;
    }

    daqdb_remove(kvs, key);
    LOG_INFO << format("Remove: [%1%]") % key.data();
    currVal = daqdb_get(kvs, key);
    LOG_INFO << format("Get: [%1%] = %2%") % key.data() % currVal.data();

    if (currVal.size()) {
        LOG_INFO << "Error: wrong value returned";
        result = false;
    }

    return result;
}
bool use_case_async_base(DaqDB::KVStoreBase *kvs) {
    bool result = true;
    const std::string expectedVal = "wxyz";
    const std::string expectedKey = "200";

    auto key = strToKey(kvs, expectedKey);
    auto val = allocValue(kvs, key, expectedVal);

    std::mutex mtx;
    std::condition_variable cv;
    bool ready = false;

    daqdb_async_put(
        kvs, key, val,
        [&](DaqDB::KVStoreBase *kvs, DaqDB::Status status, const char *key,
            const size_t keySize, const char *value, const size_t valueSize) {
            std::unique_lock<std::mutex> lck(mtx);
            if (status.ok()) {
                LOG_INFO << boost::format("PutAsync: [%1%]") % key;
            } else {
                LOG_INFO << boost::format("Error: cannot put element: %1%") %
                                status.to_string();
                result = false;
            }
            ready = true;
            cv.notify_all();
        });

    // wait for completion
    {
        std::unique_lock<std::mutex> lk(mtx);
        cv.wait_for(lk, 1s, [&ready] { return ready; });
        ready = false;
    }

    auto currVal = daqdb_get(kvs, key);
    LOG_INFO << format("Get: [%1%] = %2%") % key.data() % currVal.data();
    if (expectedVal.compare(currVal.data()) != 0) {
        LOG_INFO << "Error: wrong value returned" << std::flush;
        result = false;
    }

    daqdb_async_get(
        kvs, key,
        [&](DaqDB::KVStoreBase *kvs, DaqDB::Status status, const char *key,
            size_t keySize, const char *value, size_t valueSize) {
            std::unique_lock<std::mutex> lck(mtx);

            if (status.ok()) {
                LOG_INFO << boost::format("GetAsync: [%1%] = %2%") % key %
                                value;
                if (expectedVal.compare(value) != 0) {
                    LOG_INFO << "Error: wrong value returned" << std::flush;
                    result = false;
                }
            } else {
                LOG_INFO << boost::format("Error: cannot get element: %1%") %
                                status.to_string();
                result = false;
            }

            ready = true;
            cv.notify_all();
        });

    // wait for completion
    {
        std::unique_lock<std::mutex> lk(mtx);
        cv.wait_for(lk, 1s, [&ready] { return ready; });
        ready = false;
    }

    daqdb_remove(kvs, key);
    LOG_INFO << format("Remove: [%1%]") % key.data();

    daqdb_async_get(
        kvs, key,
        [&](DaqDB::KVStoreBase *kvs, DaqDB::Status status, const char *key,
            size_t keySize, const char *value, size_t valueSize) {
            std::unique_lock<std::mutex> lck(mtx);

            if (status.ok()) {
                LOG_INFO << boost::format("Error GetAsync found removed "
                                          "element: [%1%] = %2%") %
                                key % value;
                result = false;
            } else {
                LOG_INFO << boost::format("Cannot get element: %1%") %
                                status.to_string();
            }

            ready = true;
            cv.notify_all();
        });

    // wait for completion
    {
        std::unique_lock<std::mutex> lk(mtx);
        cv.wait_for(lk, 1s, [&ready] { return ready; });
        ready = false;
    }
    currVal = daqdb_get(kvs, key);
    LOG_INFO << format("Get: [%1%] = %2%") % key.data() % currVal.data();
    if (currVal.size()) {
        LOG_INFO << "Error: wrong value returned";
        result = false;
    }

    return result;
}
