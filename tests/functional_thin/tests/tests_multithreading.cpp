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

#include <future>
#include <thread>

#include "../base_operations.h"
#include "tests.h"

using namespace std;
using namespace DaqDB;

bool threadTask(KVStoreBase *kvs, uint64_t keyId) {
    bool result = true;
    const string val = "daqdb";

    remote_put(kvs, keyId, val);

    auto resultVal = remote_get(kvs, keyId);
    if (!checkValue(val, &resultVal)) {
        result = false;
    }

    auto removeResult = remote_remove(kvs, keyId);
    if (!removeResult) {
        result = false;
    }

    return result;
}

bool testMultithredingPutGet(KVStoreBase *kvs, Options *options) {
    bool result = true;

    future<bool> future1 = async(launch::async, threadTask, kvs, 12345);
    future<bool> future2 = async(launch::async, threadTask, kvs, 54321);

    if (!(future1.get() && future2.get())) {
        result = false;
    }
    return result;
}
