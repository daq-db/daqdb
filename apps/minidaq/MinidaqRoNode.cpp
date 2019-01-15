/**
 * Copyright 2018-2019 Intel Corporation.
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

#include "MinidaqRoNode.h"

namespace DaqDB {

thread_local int MinidaqRoNode::_eventId;

MinidaqRoNode::MinidaqRoNode(KVStoreBase *kvs) : MinidaqNode(kvs) {}

MinidaqRoNode::~MinidaqRoNode() {}

std::string MinidaqRoNode::_GetType() { return std::string("readout"); }

void MinidaqRoNode::_Setup(int executorId) { _eventId = executorId; }

Key MinidaqRoNode::_NextKey() {
    Key key = _kvs->AllocKey();
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
    }
    catch (...) {
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
