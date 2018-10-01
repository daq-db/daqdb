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

#include "MinidaqFfNode.h"
#include <random>

namespace DaqDB {

/** @todo conisder moving to a new MinidaqSelector class */
thread_local std::mt19937 _generator;

MinidaqFfNode::MinidaqFfNode(KVStoreBase *kvs) : MinidaqNode(kvs) {}

MinidaqFfNode::~MinidaqFfNode() {}

std::string MinidaqFfNode::_GetType() { return std::string("fast-filtering"); }

void MinidaqFfNode::_Setup(int executorId, MinidaqKey &key) {
    key.runId = _runId;
    key.eventId = executorId;
}

void MinidaqFfNode::_NextKey(MinidaqKey &key) {
    /** @todo getNext */
    /** @todo fixed next with distributed version
     *  (node_id, executor_id, n_global_executors)
     */
    key.eventId += _nTh;
}

bool MinidaqFfNode::_Accept() {
    bool accept = false;
    if (_acceptLevel >= 1.0) {
        accept = true;
    } else if (_acceptLevel > 0.0) {
        static thread_local std::bernoulli_distribution distro(_acceptLevel);
        accept = distro(_generator);
    }
    return accept;
}

int MinidaqFfNode::_PickSubdetector() {
    /** @todo add distro */
    return _baseId;
}

int MinidaqFfNode::_PickNFragments() {
    /** @todo add distro */
    return _nSubdetectors;
}

void MinidaqFfNode::_Task(MinidaqKey &key, std::atomic<std::uint64_t> &cnt,
                          std::atomic<std::uint64_t> &cntErr) {
    int baseId = _PickSubdetector();
    bool accept = _Accept();
    MinidaqKey *keyTmp;

    if (accept) {
        keyTmp = new MinidaqKey;
        *keyTmp = key;
    } else {
        keyTmp = &key;
    }

    Key fogKey(reinterpret_cast<char *>(keyTmp), sizeof(*keyTmp));

    for (int i = 0; i < _PickNFragments(); i++) {
        /** @todo change to GetRange once implemented */
        key.subdetectorId = baseId + i;
        DaqDB::Value value = _kvs->Get(fogKey);
#ifdef WITH_INTEGRITY_CHECK
        _CheckValue(key, value);
#endif /* WITH_INTEGRITY_CHECK */
        if (accept) {
            while (1) {
                try {
                    _kvs->UpdateAsync(
                        std::move(fogKey), UpdateOptions(LONG_TERM),
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
                    delete value.data();
                    throw;
                }
                break;
            }
        } else {
            _kvs->Remove(fogKey);
            cnt++;
        }
        delete value.data();
    }
}

void MinidaqFfNode::SetBaseSubdetectorId(int id) { _baseId = id; }

void MinidaqFfNode::SetSubdetectors(int n) { _nSubdetectors = n; }

void MinidaqFfNode::SetAcceptLevel(double p) { _acceptLevel = p; }
}
