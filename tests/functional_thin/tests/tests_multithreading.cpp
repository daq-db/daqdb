/**
 * Copyright 2019 Intel Corporation.
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

#include <future>
#include <thread>

#include "common.h"
#include "tests.h"

using namespace std;
using namespace DaqDB;

bool threadTask(KVStoreBase *kvs, string key) {
    bool result = true;
    const string expectedKey = key;
    const string expectedVal = "daqdb";

    auto putKey = strToKey(kvs, expectedKey);
    auto val = allocValue(kvs, putKey, expectedVal);

    remote_put(kvs, move(putKey), val);

    auto getKey = strToKey(kvs, expectedKey, KeyAttribute::NOT_BUFFERED);
    auto resultVal = remote_get(kvs, getKey);
    if (resultVal.data()) {
        if (expectedVal.compare(resultVal.data()) != 0) {
            result = false;
        }
    } else {
        result = false;
    }

    return result;
}

bool testMultithredingPutGet(KVStoreBase *kvs, Options *options) {
    bool result = true;

    future<bool> future1 = async(launch::async, threadTask, kvs, "12345");
    future<bool> future2 = async(launch::async, threadTask, kvs, "54321");

    if (!(future1.get() && future2.get())) {
        result = false;
    }
    return result;
}
