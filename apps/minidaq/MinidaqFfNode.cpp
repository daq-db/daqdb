/**
 * Copyright 2018, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of the copyright holder nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <random>
#include "MinidaqFfNode.h"

namespace FogKV {

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
    Key fogKey(reinterpret_cast<char *>(&key), sizeof(key));
    for (int i = 0; i < _PickNFragments(); i++) {
        /** @todo change to GetRange once implemented */
        key.subdetectorId = baseId + i;
        FogKV::Value value = _kvs->Get(fogKey);
        if (*(reinterpret_cast<uint64_t *>(value.data())) != key.eventId) {
            cntErr++;
        } else {
            cnt++;
            if (_Accept()) {
                /** @todo update */
            } else {
                //_kvs->Remove(fogKey);
            }
        }
        delete value.data();
    }
}

void MinidaqFfNode::SetBaseSubdetectorId(int id) { _baseId = id; }

void MinidaqFfNode::SetSubdetectors(int n) { _nSubdetectors = n; }

void MinidaqFfNode::SetAcceptLevel(double p) { _acceptLevel = p; }
}
