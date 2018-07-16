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

#include "OffloadRqstPooler.h"
#include "FogKV/Status.h"

#include <iostream>
#include <pthread.h>
#include <string>

#include "../debug/Logger.h"
#include "spdk/env.h"

namespace FogKV {

OffloadRqstMsg::OffloadRqstMsg(const OffloadRqstOperation op, const char *key,
                               const size_t keySize, const char *value,
                               size_t valueSize,
                               KVStoreBase::KVStoreBaseCallback clb)
    : op(op), key(key), keySize(keySize), value(value), valueSize(valueSize),
      clb(clb) {}

OffloadRqstPooler::OffloadRqstPooler() {
    rqstRing = spdk_ring_create(SPDK_RING_TYPE_MP_SC, 4096 * 4,
                                SPDK_ENV_SOCKET_ID_ANY);

    _bdev = spdk_bdev_first();
    if (_bdev == nullptr)
        FOG_DEBUG("No NVMe devices detected!");
}

OffloadRqstPooler::~OffloadRqstPooler() {
    spdk_ring_free(rqstRing);
}

bool OffloadRqstPooler::EnqueueMsg(OffloadRqstMsg *Message) {
    size_t count = spdk_ring_enqueue(rqstRing, (void **)&Message, 1);
    return (count == 1);
}

void OffloadRqstPooler::DequeueMsg() {
    processArrayCount = spdk_ring_dequeue(rqstRing, (void **)processArray,
                                          OFFLOAD_DEQUEUE_RING_LIMIT);
    assert(processArrayCount <= OFFLOAD_DEQUEUE_RING_LIMIT);
}

void OffloadRqstPooler::ProcessMsg() {
    for (unsigned short MsgIndex = 0; MsgIndex < processArrayCount;
         MsgIndex++) {
        const char *key = processArray[MsgIndex]->key;
        const size_t keySize = processArray[MsgIndex]->keySize;

        auto cb_fn = processArray[MsgIndex]->clb;

        switch (processArray[MsgIndex]->op) {
        case OffloadRqstOperation::PUT: {
            if (cb_fn)
                cb_fn(nullptr, StatusCode::NotImplemented, key, keySize,
                      nullptr, 0);
            break;
        }
        case OffloadRqstOperation::GET: {
            if (cb_fn)
                cb_fn(nullptr, StatusCode::NotImplemented, key, keySize,
                      nullptr, 0);
            break;
        }
        case OffloadRqstOperation::UPDATE: {
            if (cb_fn)

                // @TODO jradtke: add disk offload logic here
                cb_fn(nullptr, StatusCode::Ok, key, keySize, nullptr, 0);
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
