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

#include <cstring>

#include <boost/assign/list_of.hpp>

#include "tests.h"
#include <base_operations.h>
#include <debug.h>

using namespace std;
using namespace DaqDB;

bool static checkValuePutGet(KVStoreBase *kvs, string keyStr,
                             size_t valueSize) {
    bool result = true;
    auto key = strToKey(kvs, keyStr);
    auto val = allocAndFillValue(kvs, key, valueSize);

    LOG_INFO << format("Put: [%1%] with size [%2%]") % key.data() % val.size();
    daqdb_put(kvs, move(key), val);

    key = strToKey(kvs, keyStr);
    auto currVal = daqdb_get(kvs, key);
    LOG_INFO << format("Get: [%1%] with size [%2%]") % key.data() %
                    currVal.size();

    if ((val.size() != currVal.size()) ||
        !memcmp(reinterpret_cast<void *>(&val),
                reinterpret_cast<void *>(&currVal), currVal.size())) {
        LOG_INFO << "Error: wrong value returned" << flush;
        result = false;
    }

    auto removeResult = daqdb_remove(kvs, key);
    LOG_INFO << format("Remove: [%1%]") % key.data();
    if (!removeResult) {
        result = false;
        LOG_INFO << format("Error: Cannot remove a key [%1%]") % key.data();
    }
    return result;
}

bool testValueSizes(KVStoreBase *kvs) {
    bool result = true;
    const string expectedKey = "111";

    vector<size_t> valSizes = {1,    8,    16,   32,    64,   127,  128,
                               129,  255,  256,  512,   1023, 1024, 1025,
                               2048, 4096, 8192, 10240, 16384};
    for (auto valSize : valSizes) {
        if (!checkValuePutGet(kvs, expectedKey, valSize)) {
            return false;
        }
    }

    return result;
}
