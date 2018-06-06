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

#include "RqstPooler.h"

#include <iostream>

#include "spdk/env.h"

namespace FogKV {

RqstMsg::RqstMsg(const RqstOperation op, const Key *key, const Value *value,
                 void *cb_fn)
    : op(op), key(key), value(value), cb_fn(cb_fn) {}

RqstPooler::RqstPooler(std::shared_ptr<FogKV::RTreeEngine> Store)
    : isRunning(0), _thread(nullptr), rtree(Store) {

    rqstRing = spdk_ring_create(SPDK_RING_TYPE_MP_SC, 4096 * 4,
                                SPDK_ENV_SOCKET_ID_ANY);

    StartThread();
}

RqstPooler::~RqstPooler() {
    spdk_ring_free(rqstRing);
    isRunning = 0;
    if (_thread != nullptr)
        _thread->join();
}

void RqstPooler::StartThread() {
    isRunning = 1;
    _thread = new std::thread(&RqstPooler::_ThreadMain, this);
}

void RqstPooler::_ThreadMain() {
    while (isRunning) {
        DequeueMsg();
        ProcessMsg();

        /** @TODO jradtke: Initial implementation, will be removed */
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

bool RqstPooler::EnqueueMsg(RqstMsg *Message) {
    size_t count = spdk_ring_enqueue(rqstRing, (void **)&Message, 1);
    return (count == 1);

    /** @TODO jradtke: Initial implementation, error handling not implemented */
}

void RqstPooler::DequeueMsg() {
    _rqstDequedCount = spdk_ring_dequeue(rqstRing, (void **)_rqstMsgBuffer,
                                         DEQUEUE_RING_LIMIT);
    assert(_rqstDequedCount < DEQUEUE_RING_LIMIT);
}

void RqstPooler::ProcessMsg() {
    for (unsigned short MsgIndex = 0; MsgIndex < _rqstDequedCount; MsgIndex++) {
        auto key = _rqstMsgBuffer[MsgIndex]->key;
        auto cb_fn = _rqstMsgBuffer[0]->cb_fn;

        switch (_rqstMsgBuffer[MsgIndex]->op) {
        case RqstOperation::PUT: {
            auto val = _rqstMsgBuffer[MsgIndex]->value;

            StatusCode rc =
                rtree->Put(key->data(), key->size(), val->data(), val->size());
            KVStoreBase::KVStoreBasePutCallback *clb_fn =
                reinterpret_cast<KVStoreBase::KVStoreBasePutCallback *>(cb_fn);
            (*clb_fn)(nullptr, Status(rc), *key, *val);
            break;
        }
        case RqstOperation::GET: {
            /** @TODO jradtke: value buffer not implemented */
            char *resultValBuffer = nullptr;
            int32_t resultValSize = 0;

            StatusCode rc = rtree->Get(0, key->data(), key->size(),
                                       resultValBuffer, &resultValSize);
            KVStoreBase::KVStoreBaseGetCallback *clb_fn =
                reinterpret_cast<KVStoreBase::KVStoreBaseGetCallback *>(cb_fn);

            FogKV::Value resultVal(resultValBuffer, resultValSize);
            (*clb_fn)(nullptr, Status(rc), *key, resultVal);
            break;
        }
        default:
            break;
        }
    }
}

} /* namespace FogKV */
