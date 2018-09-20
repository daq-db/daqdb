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

#include <iostream>
#include <pthread.h>
#include <string>

#include <boost/asio.hpp>
#include <boost/filesystem.hpp>

#include "spdk/bdev.h"
#include "spdk/env.h"

#include "OffloadPooler.h"
#include <Logger.h>
#include <RTree.h>
#include <daqdb/Status.h>

#define POOL_FREELIST_FILENAME "/mnt/pmem/offload_free.pm"
#define POOL_FREELIST_SIZE 1ULL * 1024 * 1024 * 1024
#define LAYOUT "queue"
#define CREATE_MODE_RW (S_IWUSR | S_IRUSR)

namespace DaqDB {

/*
 * Callback function for write io completion.
 */
static void write_complete(struct spdk_bdev_io *bdev_io, bool success,
                           void *cb_arg) {

    IoCtx *ioCtx = reinterpret_cast<IoCtx *>(cb_arg);

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
    IoCtx *ioCtx = reinterpret_cast<IoCtx *>(cb_arg);

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

OffloadPooler::OffloadPooler(std::shared_ptr<DaqDB::RTreeEngine> &rtree,
                             BdevCtx &bdevCtx, uint64_t offloadBlockSize)
    : rtree(rtree), _bdevCtx(bdevCtx),
      requests(OFFLOAD_DEQUEUE_RING_LIMIT, 0) {
    rqstRing = spdk_ring_create(SPDK_RING_TYPE_MP_SC, 4096 * 4,
                                SPDK_ENV_SOCKET_ID_ANY);

    if (_bdevCtx.blk_size > 0) {
        auto aligned =
            offloadBlockSize + _bdevCtx.blk_size - 1 & ~(_bdevCtx.blk_size - 1);
        _blkForLba = aligned / _bdevCtx.blk_size;
    }
}

OffloadPooler::~OffloadPooler() { spdk_ring_free(rqstRing); }

bool OffloadPooler::Enqueue(OffloadRqst *rqst) {
    size_t count = spdk_ring_enqueue(rqstRing, (void **)&rqst, 1);
    return (count == 1);
}

bool OffloadPooler::Read(IoCtx *ioCtx) {
    return spdk_bdev_read_blocks(_bdevCtx.bdev_desc, _bdevCtx.io_channel,
                                 ioCtx->buff, *ioCtx->lba * _blkForLba,
                                 ioCtx->size, read_complete, ioCtx);
}

bool OffloadPooler::Write(IoCtx *ioCtx) {
    return spdk_bdev_write_blocks(_bdevCtx.bdev_desc, _bdevCtx.io_channel,
                                  ioCtx->buff, *ioCtx->lba * _blkForLba,
                                  ioCtx->size, write_complete, ioCtx);
}

void OffloadPooler::InitFreeList() {
    auto initNeeded = false;
    if (_bdevCtx.bdev != nullptr) {
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
        freeLbaList->maxLba = _bdevCtx.blk_num / _blkForLba;
        if (initNeeded) {
            freeLbaList->Push(_poolFreeList, -1);
        }
    }
}

void OffloadPooler::Dequeue() {
    requestCount = spdk_ring_dequeue(rqstRing, (void **)&requests[0],
                                     OFFLOAD_DEQUEUE_RING_LIMIT);
    assert(requestCount <= OFFLOAD_DEQUEUE_RING_LIMIT);
}

StatusCode OffloadPooler::_GetValCtx(const OffloadRqst *rqst,
                                     ValCtx &valCtx) const {
    return rtree->Get(rqst->key, rqst->keySize, &valCtx.val, &valCtx.size,
                      &valCtx.location);
}

void OffloadPooler::_ProcessGet(const OffloadRqst *rqst) {
    ValCtx valCtx;

    auto rc = _GetValCtx(rqst, valCtx);
    if (rc != StatusCode::Ok) {
        _RqstClb(rqst, rc);
        return;
    }

    if (_ValInPmem(valCtx)) {
        _RqstClb(rqst, StatusCode::KeyNotFound);
        return;
    }

    auto size = _GetAlignedSize(valCtx);

    char *buff = (char *)spdk_dma_zmalloc(size, _bdevCtx.buf_align, NULL);

    IoCtx *ioCtx = new IoCtx{buff,
                             _GetSizeInBlk(size),
                             rqst->key,
                             rqst->keySize,
                             static_cast<uint64_t *>(valCtx.val),
                             false,
                             rtree,
                             rqst->clb};
    if (Read(ioCtx))
        _RqstClb(rqst, StatusCode::UnknownError);
}

void OffloadPooler::_ProcessUpdate(const OffloadRqst *rqst) {
    IoCtx *ioCtx = nullptr;
    ValCtx valCtx;

    auto rc = _GetValCtx(rqst, valCtx);
    if (rc != StatusCode::Ok) {
        _RqstClb(rqst, rc);
        return;
    }

    if (_ValInPmem(valCtx)) {

        const char *val;
        size_t valSize = 0;
        if (valCtx.size > 0) {
            val = rqst->value;
            valSize = rqst->valueSize;
        } else {
            val = static_cast<char *>(valCtx.val);
            valSize = valCtx.size;
        }

        auto valSizeAlign = _GetAlignedSize(valSize);
        auto buff = reinterpret_cast<char *>(
            spdk_dma_zmalloc(valSizeAlign, _bdevCtx.buf_align, NULL));

        memcpy(buff, val, valSize);
        ioCtx = new IoCtx{buff,      _GetSizeInBlk(valSizeAlign),
                          rqst->key, rqst->keySize,
                          nullptr,   true,
                          rtree,     rqst->clb};
        rtree->AllocateIOVForKey(rqst->key, &ioCtx->lba, sizeof(uint64_t));
        *ioCtx->lba = GetFreeLba();

    } else if (_ValOffloaded(valCtx)) {
        if (valCtx.size == 0) {
            _RqstClb(rqst, StatusCode::Ok);
            return;
        }
        auto valSizeAlign = _GetAlignedSize(rqst->valueSize);
        auto buff = reinterpret_cast<char *>(
            spdk_dma_zmalloc(valSizeAlign, _bdevCtx.buf_align, NULL));
        memcpy(buff, rqst->value, rqst->valueSize);

        ioCtx = new IoCtx{buff,         _GetSizeInBlk(valSizeAlign),
                          rqst->key,    rqst->keySize,
                          new uint64_t, false,
                          rtree,        rqst->clb};
        *ioCtx->lba = *(static_cast<uint64_t *>(valCtx.val));

    } else {
        _RqstClb(rqst, StatusCode::KeyNotFound);
        return;
    }

    if (Write(ioCtx)) {
        _RqstClb(rqst, StatusCode::UnknownError);
    }
}

void OffloadPooler::_ProcessRemove(const OffloadRqst *rqst) {

    ValCtx valCtx;

    auto rc = _GetValCtx(rqst, valCtx);
    if (rc != StatusCode::Ok) {
        _RqstClb(rqst, rc);
        return;
    }


    if (_ValInPmem(valCtx)) {
        _RqstClb(rqst, StatusCode::KeyNotFound);
        return;
    }

    uint64_t lba = *(static_cast<uint64_t *>(valCtx.val));

    freeLbaList->Push(_poolFreeList, lba);

    rtree->Remove(rqst->key);

    _RqstClb(rqst, StatusCode::Ok);
}

void OffloadPooler::Process() {
    for (unsigned short RqstIdx = 0; RqstIdx < requestCount; RqstIdx++) {
        auto rqst = requests[RqstIdx];

        switch (requests[RqstIdx]->op) {
        case OffloadOperation::GET:
            _ProcessGet(const_cast<const OffloadRqst*>(rqst));
            break;
        case OffloadOperation::UPDATE: {
            _ProcessUpdate(const_cast<const OffloadRqst*>(rqst));
            break;
        }
        case OffloadOperation::REMOVE: {
            _ProcessRemove(const_cast<const OffloadRqst*>(rqst));
            break;
        }
        default:
            break;
        }
        delete requests[RqstIdx];
    }
    requestCount = 0;
}

int64_t OffloadPooler::GetFreeLba() { return freeLbaList->Get(_poolFreeList); }

void OffloadPooler::SetBdevContext(BdevCtx &bdevContext) {
    _bdevCtx = bdevContext;
}

} // namespace DaqDB
