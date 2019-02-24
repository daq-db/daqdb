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

#include "MinidaqAroNode.h"

namespace DaqDB {

MinidaqAroNode::MinidaqAroNode(KVStoreBase *kvs) : MinidaqRoNode(kvs) {}

MinidaqAroNode::~MinidaqAroNode() {}

std::string MinidaqAroNode::_GetType() { return std::string("readout-async"); }

void MinidaqAroNode::_Task(Key &&key, std::atomic<std::uint64_t> &cnt,
                           std::atomic<std::uint64_t> &cntErr) {
    DaqDB::Value value;
    try {
        value = _kvs->Alloc(key, _fSize);
    }
    catch (...) {
        _kvs->Free(std::move(key));
        throw;
    }

    memcpy(value.data(), _data_buffer, value.size());

#ifdef WITH_INTEGRITY_CHECK
    _FillBuffer(key, value.data(), value.size());
#endif /* WITH_INTEGRITY_CHECK */

    _kvs->PutAsync(std::move(key), std::move(value),
                   [&cnt, &cntErr](DaqDB::KVStoreBase *kvs,
                                   DaqDB::Status status, const char *key,
                                   const size_t keySize, const char *value,
                                   const size_t valueSize) {
        if (!status.ok()) {
            cntErr++;
        } else {
            cnt++;
        }
    });
}
}
