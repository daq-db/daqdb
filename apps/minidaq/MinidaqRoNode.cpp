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

MinidaqRoNode::MinidaqRoNode(KVStoreBase *kvs) : MinidaqNode(kvs) {}

MinidaqRoNode::~MinidaqRoNode() {}

std::string MinidaqRoNode::_GetType() { return std::string("readout"); }

void MinidaqRoNode::_Setup(int executorId, MinidaqKey &key) {
    key.subdetectorId = _id;
    key.runId = _runId;
    key.eventId = executorId;
}

void MinidaqRoNode::_NextKey(MinidaqKey &key) { key.eventId += _nTh; }

void MinidaqRoNode::_Task(MinidaqKey &key, std::atomic<std::uint64_t> &cnt,
                          std::atomic<std::uint64_t> &cntErr) {
    Key fogKey(reinterpret_cast<char *>(&key), sizeof(key));
    DaqDB::Value value = _kvs->Alloc(fogKey, _fSize);

#ifdef WITH_INTEGRITY_CHECK
    _FillBuffer(key, value.data(), value.size());
#endif /* WITH_INTEGRITY_CHECK */

    _kvs->Put(std::move(fogKey), std::move(value));
    cnt++;
}

void MinidaqRoNode::SetFragmentSize(size_t s) { _fSize = s; }

void MinidaqRoNode::SetSubdetectorId(int id) { _id = id; }
}
