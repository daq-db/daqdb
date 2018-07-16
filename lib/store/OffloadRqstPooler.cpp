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
#include "spdk_internal/bdev.h"

namespace FogKV {

/*
 * Callback function for write io completion.
 */
static void write_complete(struct spdk_bdev_io *bdev_io, bool success,
                           void *cb_arg) {

    IoContext *ioCtx = (IoContext *)cb_arg;

    if (success) {
        if (ioCtx->clb)
            ioCtx->clb(nullptr, StatusCode::Ok, ioCtx->key, ioCtx->keySize,
                       ioCtx->buff, ioCtx->size);
    } else {
        if (ioCtx->clb)
            ioCtx->clb(nullptr, StatusCode::UnknownError, ioCtx->key,
                       ioCtx->keySize, nullptr, 0);
    }

    /* Complete the I/O */
    spdk_bdev_free_io(bdev_io);
}

/*
 * Callback function for read io completion.
 */
static void read_complete(struct spdk_bdev_io *bdev_io, bool success,
                          void *cb_arg) {
    IoContext *ioCtx = (IoContext *)cb_arg;

    if (success) {
        if (ioCtx->clb)
            ioCtx->clb(nullptr, StatusCode::Ok, ioCtx->key, ioCtx->keySize,
                       ioCtx->buff, ioCtx->size);
    } else {
        if (ioCtx->clb)
            ioCtx->clb(nullptr, StatusCode::UnknownError, ioCtx->key,
                       ioCtx->keySize, nullptr, 0);
    }

    spdk_bdev_free_io(bdev_io);
}

OffloadRqstMsg::OffloadRqstMsg(const OffloadRqstOperation op, const char *key,
                               const size_t keySize, const char *value,
                               size_t valueSize,
                               KVStoreBase::KVStoreBaseCallback clb)
    : op(op), key(key), keySize(keySize), value(value), valueSize(valueSize),
      clb(clb) {}

OffloadRqstPooler::OffloadRqstPooler(BdevContext &bdevContext)
    : _bdevContext(bdevContext) {
    rqstRing = spdk_ring_create(SPDK_RING_TYPE_MP_SC, 4096 * 4,
                                SPDK_ENV_SOCKET_ID_ANY);
}

OffloadRqstPooler::~OffloadRqstPooler() { spdk_ring_free(rqstRing); }

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

            char *buff = (char *)spdk_dma_zmalloc(_bdevContext.blk_size,
                                                  _bdevContext.buf_align, NULL);
            IoContext *ioCtx =
                new IoContext{buff, _bdevContext.blk_size, key, keySize, cb_fn};
            int rc = spdk_bdev_read(
                _bdevContext.bdev_desc, _bdevContext.bdev_io_channel, buff, 0,
                _bdevContext.blk_size, read_complete, ioCtx);

            break;
        }
        case OffloadRqstOperation::UPDATE: {
            const char *val = processArray[MsgIndex]->value;
            const size_t valSize = processArray[MsgIndex]->valueSize;

            char *buff =
                (char *)spdk_dma_zmalloc(valSize, _bdevContext.buf_align, NULL);
            memcpy(buff, val, valSize);
            IoContext *ioCtx =
                new IoContext{buff, _bdevContext.blk_size, key, keySize, cb_fn};

            int rc = spdk_bdev_write(
                _bdevContext.bdev_desc, _bdevContext.bdev_io_channel, buff, 0,
                _bdevContext.blk_size, write_complete, ioCtx);

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
