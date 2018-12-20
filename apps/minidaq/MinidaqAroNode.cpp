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

#include "MinidaqAroNode.h"

namespace DaqDB {

MinidaqAroNode::MinidaqAroNode(KVStoreBase *kvs) : MinidaqRoNode(kvs) {}

MinidaqAroNode::~MinidaqAroNode() {}

std::string MinidaqAroNode::_GetType() { return std::string("readout-async"); }

void MinidaqAroNode::_Task(Key &&key, std::atomic<std::uint64_t> &cnt,
                           std::atomic<std::uint64_t> &cntErr) {

    MinidaqKey *keyTmp = new MinidaqKey;
    *keyTmp = _mKey;
    key = Key(reinterpret_cast<char *>(keyTmp), sizeof(*keyTmp));
    DaqDB::Value value = _kvs->Alloc(key, _fSize);

#ifdef WITH_INTEGRITY_CHECK
    _FillBuffer(key, value.data(), value.size());
#endif /* WITH_INTEGRITY_CHECK */

    while (1) {
        try {
            _kvs->PutAsync(std::move(key), std::move(value),
                           [keyTmp, &cnt, &cntErr](
                               DaqDB::KVStoreBase *kvs, DaqDB::Status status,
                               const char *key, const size_t keySize,
                               const char *value, const size_t valueSize) {
                               if (!status.ok()) {
                                   cntErr++;
                               } else {
                                   cnt++;
                               }
                               delete keyTmp;
                           });
        } catch (QueueFullException &e) {
            // Keep retrying
            continue;
        } catch (...) {
            delete keyTmp;
            throw;
        }
        break;
    }
}
}
