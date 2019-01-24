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

#include "common.h"
#include "tests.h"

using namespace std;
using namespace DaqDB;

bool static checkValuePutGet(KVStoreBase *kvs, const string keyStr,
                             size_t valueSize) {
    bool result = true;
    auto putKey = strToKey(kvs, keyStr);
    auto val = allocAndFillValue(kvs, putKey, valueSize);

    LOG_INFO << format("Remote Put: [%1%] with size [%2%]") % putKey.data() %
                    val.size();
    remote_put(kvs, move(putKey), val);

    auto key = strToKey(kvs, keyStr, KeyValAttribute::NOT_BUFFERED);

    auto resultVal = remote_get(kvs, key);
    if (resultVal.data()) {
        LOG_INFO << format("Remote Get: [%1%] with size [%2%]") % key.data() %
                        resultVal.size();
    }

    if ((val.size() != resultVal.size()) ||
        !memcmp(reinterpret_cast<void *>(&val),
                reinterpret_cast<void *>(&resultVal), resultVal.size())) {
        LOG_INFO << "Error: wrong value returned" << flush;
        result = false;
    }

    LOG_INFO << format("Remote Remove: [%1%]") % key.data();
    auto removeResult = remote_remove(kvs, key);
    if (!removeResult) {
        result = false;
        LOG_INFO << format("Error: Cannot remove a key [%1%]") % key.data();
    }

    return result;
}

bool testValueSizes(KVStoreBase *kvs, DaqDB::Options *options) {
    bool result = true;

    const string expectedKey = "200";

    vector<size_t> valSizes = {8,    16,   32,   64,   127,   128,
                               129,  255,  256,  512,  1023,  1024,
                               1025, 2048, 4096, 8192, 10240, 16384};

    for (auto valSize : valSizes) {
        if (!checkValuePutGet(kvs, expectedKey, valSize)) {
            return false;
        }
    }

    return result;
}
