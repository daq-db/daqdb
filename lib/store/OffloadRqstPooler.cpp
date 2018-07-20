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
#include "RTree.h"

#include <boost/asio.hpp>
#include <iostream>
#include <pthread.h>
#include <string>

#include <boost/filesystem.hpp>

#include "../debug/Logger.h"
#include "spdk/env.h"
#include "spdk_internal/bdev.h"

#define POOL_FREELIST_FILENAME "/mnt/pmem/offload_free.pm"
#define POOL_FREELIST_SIZE 1ULL * 1024 * 1024 * 1024
#define LAYOUT "queue"
#define CREATE_MODE_RW (S_IWUSR | S_IRUSR)

namespace FogKV {

/*
 * Callback function for write io completion.
 */
static void write_complete(struct spdk_bdev_io *bdev_io, bool success,
                           void *cb_arg) {

    IoContext *ioCtx = (IoContext *)cb_arg;

    if (success) {
        if (ioCtx->updatePmemIOV)
            ioCtx->rtree->UpdateValueWrapper(ioCtx->key, ioCtx->lba,
                                             sizeof(uint64_t));
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

OffloadRqstPooler::OffloadRqstPooler(std::shared_ptr<FogKV::RTreeEngine> rtree,
                                     BdevContext &bdevContext,
                                     uint64_t offloadBlockSize)
    : rtree(rtree), _bdevContext(bdevContext) {
    rqstRing = spdk_ring_create(SPDK_RING_TYPE_MP_SC, 4096 * 4,
                                SPDK_ENV_SOCKET_ID_ANY);

    if (_bdevContext.blk_size > 0) {
        auto aligned = offloadBlockSize + _bdevContext.blk_size - 1 &
                       ~(_bdevContext.blk_size - 1);
        _blkForLba = aligned / _bdevContext.blk_size;
    }
}

OffloadRqstPooler::~OffloadRqstPooler() { spdk_ring_free(rqstRing); }

bool OffloadRqstPooler::EnqueueMsg(OffloadRqstMsg *Message) {
    size_t count = spdk_ring_enqueue(rqstRing, (void **)&Message, 1);
    return (count == 1);
}

bool OffloadRqstPooler::Read(IoContext *ioCtx) {
    return spdk_bdev_read_blocks(
        _bdevContext.bdev_desc, _bdevContext.bdev_io_channel, ioCtx->buff,
        *ioCtx->lba * _blkForLba, ioCtx->size, read_complete, ioCtx);
}

bool OffloadRqstPooler::Write(IoContext *ioCtx) {
    return spdk_bdev_write_blocks(
        _bdevContext.bdev_desc, _bdevContext.bdev_io_channel, ioCtx->buff,
        *ioCtx->lba * _blkForLba, ioCtx->size, write_complete, ioCtx);
}

void OffloadRqstPooler::InitFreeList() {
    auto initNeeded = false;
    if (_bdevContext.bdev != nullptr) {
        auto needInit = false;
        if (boost::filesystem::exists(POOL_FREELIST_FILENAME)) {
            _poolFreeList =
                pool<OffloadFreeList>::open(POOL_FREELIST_FILENAME, LAYOUT);
        } else {
            _poolFreeList = pool<OffloadFreeList>::create(
                POOL_FREELIST_FILENAME, LAYOUT, POOL_FREELIST_SIZE,
                CREATE_MODE_RW);
            initNeeded = true;
        }
        freeLbaList = _poolFreeList.get_root().get();
        freeLbaList->maxLba = _bdevContext.blk_num / _blkForLba;
        if (initNeeded) {
            freeLbaList->Push(_poolFreeList, -1);
        }
    }
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

        /**
         * First we need discover if value is not already offloaded
         */
        void *rtreeVal;
        size_t rtreeValSize;
        uint8_t location;
        StatusCode rc =
            rtree->Get(key, keySize, &rtreeVal, &rtreeValSize, &location);
        if (rc != StatusCode::Ok) {
            if (cb_fn)
                cb_fn(nullptr, rc, key, keySize, nullptr, 0);
            break;
        }

        switch (processArray[MsgIndex]->op) {
        case OffloadRqstOperation::GET: {

            if (location == LOCATIONS::PMEM) {
                if (cb_fn)
                    cb_fn(nullptr, StatusCode::KeyNotFound, key, keySize,
                          nullptr, 0);
                break;
            }
            auto alignedSize = rtreeValSize + _bdevContext.blk_size - 1 &
                               ~(_bdevContext.blk_size - 1);
            uint32_t sizeInBlk = alignedSize / _bdevContext.blk_size;

            char *buff = (char *)spdk_dma_zmalloc(alignedSize,
                                                  _bdevContext.buf_align, NULL);

            IoContext *ioCtx = new IoContext{buff,
                                             sizeInBlk,
                                             key,
                                             keySize,
                                             static_cast<uint64_t *>(rtreeVal),
                                             false,
                                             rtree,
                                             cb_fn};
            if (Read(ioCtx)) {
                if (cb_fn)
                    cb_fn(nullptr, StatusCode::UnknownError, key, keySize,
                          nullptr, 0);
            }
            break;
        }
        case OffloadRqstOperation::UPDATE: {

            const char *rqstVal = processArray[MsgIndex]->value;
            size_t rqstValSize = processArray[MsgIndex]->valueSize;
            char *buff = nullptr;
            IoContext *ioCtx = nullptr;
            if (location == LOCATIONS::PMEM) {
                const char *val;
                size_t valSize;
                if (rqstValSize > 0) {
                    val = rqstVal;
                    valSize = rtreeValSize;
                } else {
                    val = static_cast<char *>(rtreeVal);
                    valSize = rtreeValSize;
                }
                auto alignedSize = valSize + _bdevContext.blk_size - 1 &
                                   ~(_bdevContext.blk_size - 1);
                uint32_t sizeInBlk = alignedSize / _bdevContext.blk_size;
                buff = (char *)spdk_dma_zmalloc(alignedSize,
                                                _bdevContext.buf_align, NULL);
                memcpy(buff, val, valSize);
                ioCtx = new IoContext{buff,    sizeInBlk, key,   keySize,
                                      nullptr, true,      rtree, cb_fn};
                rtree->AllocateIOVForKey(key, &ioCtx->lba, sizeof(uint64_t));
                *ioCtx->lba = GetFreeLba();
            } else if (location == LOCATIONS::DISK) {
                if (rqstValSize == 0) {
                    if (cb_fn)
                        cb_fn(nullptr, StatusCode::Ok, key, keySize, nullptr,
                              0);
                    break;
                }
                buff = (char *)spdk_dma_zmalloc(rqstValSize,
                                                _bdevContext.buf_align, NULL);
                memcpy(buff, rqstVal, rqstValSize);

                auto alignedSize = rqstValSize + _bdevContext.blk_size - 1 &
                                   ~(_bdevContext.blk_size - 1);
                uint32_t sizeInBlk = _blkForLba =
                    alignedSize / _bdevContext.blk_size;

                ioCtx = new IoContext{buff,         sizeInBlk, key,   keySize,
                                      new uint64_t, false,     rtree, cb_fn};
                *ioCtx->lba = *(static_cast<uint64_t *>(rtreeVal));
            } else {
                if (cb_fn)
                    cb_fn(nullptr, StatusCode::KeyNotFound, key, keySize,
                          nullptr, 0);
            }

            if (Write(ioCtx)) {
                if (cb_fn)
                    cb_fn(nullptr, StatusCode::UnknownError, key, keySize,
                          nullptr, 0);
            }
            break;
        }
        case OffloadRqstOperation::REMOVE: {

            if (location == LOCATIONS::PMEM) {
                if (cb_fn)
                    cb_fn(nullptr, StatusCode::KeyNotFound, key, keySize,
                          nullptr, 0);
                break;
            }

            uint64_t lba = *(static_cast<uint64_t *>(rtreeVal));
            freeLbaList->Push(_poolFreeList, lba);
            rtree->Remove(key);

            if (cb_fn)
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

int64_t OffloadRqstPooler::GetFreeLba() {
    return freeLbaList->GetFreeLba(_poolFreeList);
}

void OffloadRqstPooler::SetBdevContext(BdevContext &bdevContext) {
    _bdevContext = bdevContext;
}

} /* namespace FogKV */
