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

#include "MinidaqRoNode.h"

namespace DaqDB {

thread_local MinidaqKey MinidaqRoNode::_mKey;

MinidaqRoNode::MinidaqRoNode(KVStoreBase *kvs) : MinidaqNode(kvs) {
}

MinidaqRoNode::~MinidaqRoNode() {}

std::string MinidaqRoNode::_GetType() { return std::string("readout"); }

void MinidaqRoNode::_Setup(int executorId) {
    _mKey.runId = _runId;
    _mKey.eventId = executorId - _nTh;
    _mKey.subdetectorId = _id;
}

Key MinidaqRoNode::_NextKey() {
    _mKey.eventId += _nTh;
    return Key(reinterpret_cast<char *>(&_mKey), sizeof(_mKey));
}

void MinidaqRoNode::_Task(Key &&key, std::atomic<std::uint64_t> &cnt,
                          std::atomic<std::uint64_t> &cntErr) {
    DaqDB::Value value = _kvs->Alloc(key, _fSize);

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
