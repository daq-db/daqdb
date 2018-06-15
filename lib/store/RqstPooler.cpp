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

RqstMsg::RqstMsg(const RqstOperation op, const char *key, const size_t keySize,
                 const char *value, size_t valueSize,
                 KVStoreBase::KVStoreBaseCallback clb)
    : op(op), key(key), keySize(keySize), value(value), valueSize(valueSize),
      clb(clb) {}

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
    }
}

bool RqstPooler::EnqueueMsg(RqstMsg *Message) {
    size_t count = spdk_ring_enqueue(rqstRing, (void **)&Message, 1);
    return (count == 1);

    /** @TODO jradtke: Initial implementation, error handling not implemented */
}

void RqstPooler::DequeueMsg() {
    processArrayCount =
        spdk_ring_dequeue(rqstRing, (void **)processArray, DEQUEUE_RING_LIMIT);
    assert(processArrayCount <= DEQUEUE_RING_LIMIT);
}

void RqstPooler::ProcessMsg() {
    for (unsigned short MsgIndex = 0; MsgIndex < processArrayCount;
         MsgIndex++) {
        const char *key = processArray[MsgIndex]->key;
        const size_t keySize = processArray[MsgIndex]->keySize;

        auto cb_fn = processArray[MsgIndex]->clb;

        switch (processArray[MsgIndex]->op) {
        case RqstOperation::PUT: {
            const char *val = processArray[MsgIndex]->value;
            const size_t valSize = processArray[MsgIndex]->valueSize;

            StatusCode rc = rtree->Put(key, keySize, val, valSize);
            if (cb_fn)
                cb_fn(nullptr, Status(rc), key, keySize, val, valSize);
            break;
        }
        case RqstOperation::GET: {
            size_t size;
            char* pVal;

            StatusCode rc = rtree->Get(key, keySize, &pVal, &size);
            if (rc != StatusCode::Ok) {
                if (cb_fn)
                    cb_fn(nullptr, Status(rc), key, keySize, nullptr, 0);
                break;
            }
            Value value(new char[size], size);
            std::memcpy(value.data(), pVal, size);
                if (cb_fn)
                    cb_fn(nullptr, Status(rc), key, keySize, value.data(), size);
            break;
        }
        default:
            break;
        }
        delete processArray[MsgIndex];
    }
    processArrayCount = 0;
}

} /* namespace FogKV */
