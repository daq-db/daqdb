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

    DAQDB_INFO << format("Put: [%1%] with size [%2%]") % key.data() %
                      val.size();
    daqdb_put(kvs, move(key), val);

    key = strToKey(kvs, keyStr);
    auto currVal = daqdb_get(kvs, key);
    DAQDB_INFO << format("Get: [%1%] with size [%2%]") % key.data() %
                      currVal.size();

    if ((val.size() != currVal.size()) ||
        !memcmp(reinterpret_cast<void *>(&val),
                reinterpret_cast<void *>(&currVal), currVal.size())) {
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
