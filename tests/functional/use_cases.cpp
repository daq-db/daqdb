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

#include "base_operations.h"
#include "debug.h"
#include "use_cases.h"

using namespace std::chrono_literals;

#define USE_CASE_LOG(name)                                                     \
    BOOST_LOG_SEV(lg::get(), bt::info) << std::endl                            \
                                       << std::string(80, '-') << std::endl    \
                                       << name << std::endl                    \
                                       << std::string(80, '-') << std::flush;

bool use_case_sync_base(std::shared_ptr<DaqDB::KVStoreBase> &spKvs) {
    bool result = true;
    const std::string expectedVal = "abcd";
    const std::string expectedKey = "100";

    auto key = strToKey(spKvs, expectedKey);
    auto val = allocValue(spKvs, key, expectedVal);

    USE_CASE_LOG("use_case_sync_base");

    daqdb_put(spKvs, key, val);
    BOOST_LOG_SEV(lg::get(), bt::info)
        << format("Put: [%1%] = %2%") % key.data() % val.data();
    auto currVal = daqdb_get(spKvs, key);
    BOOST_LOG_SEV(lg::get(), bt::info)
        << format("Get: [%1%] = %2%") % key.data() % currVal.data();

    if (expectedVal.compare(currVal.data()) != 0) {
        BOOST_LOG_SEV(lg::get(), bt::info)
            << "Error: wrong value returned" << std::flush;
        result = false;
    }

    daqdb_remove(spKvs, key);
    BOOST_LOG_SEV(lg::get(), bt::info) << format("Remove: [%1%]") % key.data();
    currVal = daqdb_get(spKvs, key);
    BOOST_LOG_SEV(lg::get(), bt::info)
        << format("Get: [%1%] = %2%") % key.data() % currVal.data();

    if (currVal.size()) {
        BOOST_LOG_SEV(lg::get(), bt::info) << "Error: wrong value returned";
        result = false;
    }

    return result;
}
bool use_case_async_base(std::shared_ptr<DaqDB::KVStoreBase> &spKvs) {
    bool result = true;
    const std::string expectedVal = "wxyz";
    const std::string expectedKey = "200";

    auto key = strToKey(spKvs, expectedKey);
    auto val = allocValue(spKvs, key, expectedVal);

    USE_CASE_LOG("use_case_async_base");

    std::mutex mtx;
    std::condition_variable cv;
    bool ready = false;

    daqdb_async_put(
        spKvs, key, val,
        [&](DaqDB::KVStoreBase *kvs, DaqDB::Status status, const char *key,
            const size_t keySize, const char *value, const size_t valueSize) {
            std::unique_lock<std::mutex> lck(mtx);
            if (status.ok()) {
                BOOST_LOG_SEV(lg::get(), bt::info)
                    << boost::format("PutAsync: [%1%]") % key;
            } else {
                BOOST_LOG_SEV(lg::get(), bt::info)
                    << boost::format("Error: cannot put element: %1%") %
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

    auto currVal = daqdb_get(spKvs, key);
    BOOST_LOG_SEV(lg::get(), bt::info)
        << format("Get: [%1%] = %2%") % key.data() % currVal.data();
    if (expectedVal.compare(currVal.data()) != 0) {
        BOOST_LOG_SEV(lg::get(), bt::info)
            << "Error: wrong value returned" << std::flush;
        result = false;
    }

    daqdb_async_get(
        spKvs, key,
        [&](DaqDB::KVStoreBase *kvs, DaqDB::Status status, const char *key,
            size_t keySize, const char *value, size_t valueSize) {
            std::unique_lock<std::mutex> lck(mtx);

            if (status.ok()) {
                BOOST_LOG_SEV(lg::get(), bt::info)
                    << boost::format("GetAsync: [%1%] = %2%") % key % value;
                if (expectedVal.compare(value) != 0) {
                    BOOST_LOG_SEV(lg::get(), bt::info)
                        << "Error: wrong value returned" << std::flush;
                    result = false;
                }
            } else {
                BOOST_LOG_SEV(lg::get(), bt::info)
                    << boost::format("Error: cannot get element: %1%") %
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

    daqdb_remove(spKvs, key);
    BOOST_LOG_SEV(lg::get(), bt::info) << format("Remove: [%1%]") % key.data();

    daqdb_async_get(
        spKvs, key,
        [&](DaqDB::KVStoreBase *kvs, DaqDB::Status status, const char *key,
            size_t keySize, const char *value, size_t valueSize) {
            std::unique_lock<std::mutex> lck(mtx);

            if (status.ok()) {
                BOOST_LOG_SEV(lg::get(), bt::info)
                    << boost::format("GetAsync: [%1%] = %2%") % key % value;
                result = false;
            } else {
                BOOST_LOG_SEV(lg::get(), bt::info)
                    << boost::format("Error: cannot get element: %1%") %
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
    currVal = daqdb_get(spKvs, key);
    BOOST_LOG_SEV(lg::get(), bt::info)
        << format("Get: [%1%] = %2%") % key.data() % currVal.data();
    if (currVal.size()) {
        BOOST_LOG_SEV(lg::get(), bt::info) << "Error: wrong value returned";
        result = false;
    }

    return result;
}

bool use_case_sync_offload(std::shared_ptr<DaqDB::KVStoreBase> &spKvs) {
    USE_CASE_LOG("use_case_sync_offload");
    return true;
}

bool use_case_async_offload(std::shared_ptr<DaqDB::KVStoreBase> &spKvs) {
    USE_CASE_LOG("use_case_async_offload");
    return true;
}
