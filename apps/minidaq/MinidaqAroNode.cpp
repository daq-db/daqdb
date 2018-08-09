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

namespace FogKV {

MinidaqAroNode::MinidaqAroNode(KVStoreBase *kvs) : MinidaqRoNode(kvs) {}

MinidaqAroNode::~MinidaqAroNode() {}

std::string MinidaqAroNode::_GetType() { return std::string("readout-async"); }

void MinidaqAroNode::_Task(MinidaqKey &key, std::atomic<std::uint64_t> &cnt,
                           std::atomic<std::uint64_t> &cntErr) {

    MinidaqKey *keyTmp = new MinidaqKey;
    *keyTmp = key;
    Key fogKey(reinterpret_cast<char *>(keyTmp), sizeof(*keyTmp));
    FogKV::Value value = _kvs->Alloc(fogKey, _fSize);
    while (1) {
        try {
            _kvs->PutAsync(std::move(fogKey), std::move(value),
                           [keyTmp, &cnt, &cntErr](
                               FogKV::KVStoreBase *kvs, FogKV::Status status,
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
