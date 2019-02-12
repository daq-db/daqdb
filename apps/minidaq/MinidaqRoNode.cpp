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

#include "MinidaqRoNode.h"

namespace DaqDB {

thread_local int MinidaqRoNode::_eventId;

MinidaqRoNode::MinidaqRoNode(KVStoreBase *kvs) : MinidaqNode(kvs) {}

MinidaqRoNode::~MinidaqRoNode() {}

std::string MinidaqRoNode::_GetType() { return std::string("readout"); }

void MinidaqRoNode::_Setup(int executorId) { _eventId = executorId; }

Key MinidaqRoNode::_NextKey() {
    Key key = _kvs->AllocKey(_localOnly
                                 ? AllocOptions(KeyValAttribute::NOT_BUFFERED)
                                 : AllocOptions(KeyValAttribute::KVS_BUFFERED));
    MinidaqKey *mKeyPtr = reinterpret_cast<MinidaqKey *>(key.data());
    mKeyPtr->runId = _runId;
    mKeyPtr->subdetectorId = _id;
    mKeyPtr->eventId = _eventId;
    _eventId += _nTh;
    return key;
}

void MinidaqRoNode::_Task(Key &&key, std::atomic<std::uint64_t> &cnt,
                          std::atomic<std::uint64_t> &cntErr) {
    DaqDB::Value value;
    try {
        value = _kvs->Alloc(key, _fSize);
    } catch (...) {
        _kvs->Free(std::move(key));
        throw;
    }

#ifdef WITH_INTEGRITY_CHECK
    _FillBuffer(key, value.data(), value.size());
    _CheckBuffer(key, value.data(), value.size());
#endif /* WITH_INTEGRITY_CHECK */

    _kvs->Put(std::move(key), std::move(value));
    cnt++;
}

void MinidaqRoNode::SetFragmentSize(size_t s) { _fSize = s; }

void MinidaqRoNode::SetSubdetectorId(int id) { _id = id; }
}
