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

#include "../base_operations.h"
#include "tests.h"

using namespace std;
using namespace DaqDB;

bool static checkValuePutGet(KVStoreBase *kvs, uint64_t keyId,
                             size_t valueSize) {
    bool result = true;
    auto val = generateValueStr(valueSize);

    remote_put(kvs, keyId, val);

    auto resultVal = remote_get(kvs, keyId);
    if (!checkValue(val, &resultVal)) {
        result = false;
    }

    auto removeResult = remote_remove(kvs, keyId);
    if (!removeResult) {
        result = false;
        DAQDB_INFO << format("Error: Cannot remove a key [%1%]") % keyId;
    }

    return result;
}

bool testValueSizes(KVStoreBase *kvs, DaqDB::Options *options) {
    bool result = true;

    uint64_t expectedKey = 200;

    vector<size_t> valSizes = {8,    16,   32,   64,   127,   128,
                               129,  255,  256,  512,  1023,  1024,
                               1025, 2048, 4096, 8192, 10240, 16384};

    for (auto valSize : valSizes) {
        if (!checkValuePutGet(kvs, expectedKey++, valSize)) {
            return false;
        }
    }

    return result;
}
