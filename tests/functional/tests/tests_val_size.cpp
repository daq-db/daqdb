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

bool static checkValuePutGet(KVStoreBase *kvs, uint64_t keyId,
                             size_t valueSize) {
    bool result = true;
    auto val = generateValueStr(valueSize);
    daqdb_put(kvs, keyId, val);

    auto resultVal = daqdb_get(kvs, keyId);

    if ((val.size() != resultVal.size()) ||
        !memcmp(reinterpret_cast<void *>(&val),
                reinterpret_cast<void *>(&resultVal), resultVal.size())) {
        DAQDB_INFO << "Error: wrong value returned" << flush;
        result = false;
    }

    auto removeResult = daqdb_remove(kvs, keyId);
    if (!removeResult) {
        result = false;
        DAQDB_INFO << format("Error: Cannot remove a key [%1%]") % keyId;
    }
    return result;
}

bool testValueSizes(KVStoreBase *kvs) {
    bool result = true;
    uint64_t expectedKey = 111;

    vector<size_t> valSizes = { 1,         8,     16,        32,
                                64,        127,   128,       129,
                                255,       256,   512,       1023,
                                1024,      1025,  2 * 1024,  4 * 1024,
                                8 * 1024,  10240, 16 * 1024, 32 * 1024,
                                64 * 1024, 69632, 128 * 1024 };

    for (auto valSize : valSizes) {
        if (!checkValuePutGet(kvs, expectedKey++, valSize)) {
            return false;
        }
    }
    return result;
}
