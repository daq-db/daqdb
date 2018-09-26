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

#include "uc.h"
#include <base_operations.h>
#include <debug.h>

using namespace std::chrono_literals;

bool use_case_sync_offload(std::shared_ptr<DaqDB::KVStoreBase> &spKvs) {
    USE_CASE_LOG("use_case_sync_offload");

    bool result = true;
    const std::string expectedVal = "fghi";
    const std::string expectedKey = "300";

    auto key = strToKey(spKvs, expectedKey);
    auto val = allocValue(spKvs, key, expectedVal);

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

    if (spKvs->IsOffloaded(key)) {
        BOOST_LOG_SEV(lg::get(), bt::info) << "Error: wrong value location";
        result = false;
    }

    daqdb_offload(spKvs, key);
    BOOST_LOG_SEV(lg::get(), bt::info) << format("Offload: [%1%]") % key.data();

    if (!spKvs->IsOffloaded(key)) {
        BOOST_LOG_SEV(lg::get(), bt::info) << "Error: wrong value location";
        result = false;
    }

    currVal = daqdb_get(spKvs, key);
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
