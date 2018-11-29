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

#include "boost/algorithm/string.hpp"

#include "common.h"
#include "tests.h"

using namespace std;
using namespace DaqDB;

bool testRemotePeerConnect(KVStoreBase *kvs) {
    bool result = true;
    auto neighbours = kvs->getProperty("daqdb.dht.neighbours");

    if (!boost::contains(neighbours, "Ready")) {
        LOG_INFO << "Cannot contact peer DHT server" << flush;
        result = false;
    }

    return result;
}

bool testPutGetSequence(KVStoreBase *kvs) {
    bool result = true;

    const string expectedVal = "daqdb";
    const string expectedKey = "1000";

    auto key = strToKey(kvs, expectedKey);
    auto val = allocValue(kvs, key, expectedVal);

    remote_put(kvs, key, val);
    LOG_INFO << format("Remote Put: [%1%] = %2%") % key.data() % val.data();

    auto resultVal = remote_get(kvs, key);
    if (resultVal.data()) {
        LOG_INFO << format("Remote Get result: [%1%] = %2%") % key.data() %
                        resultVal.data();
    }

    if (!resultVal.data() || expectedVal.compare(resultVal.data()) != 0) {
        result = false;
        LOG_INFO << "Error: wrong value returned";
    }

    auto removeResult = remote_remove(kvs, key);
    LOG_INFO << format("Remote Remove: [%1%]") % key.data();
    if (!removeResult) {
        result = false;
        LOG_INFO << format("Error: Cannot remove a key [%1%]") % key.data();
    }

    return result;
}
