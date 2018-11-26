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
#include <thread>

#include <boost/asio.hpp>
#include <boost/filesystem.hpp>

#include "spdk/bdev.h"
#include "spdk/env.h"
#include "spdk/queue.h"
#include "spdk/thread.h"
#include "spdk/util.h"

#include "OffloadPoller.h"
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

    OffloadIoCtx *ioCtx = reinterpret_cast<OffloadIoCtx *>(cb_arg);

    if (success) {
        if (ioCtx->updatePmemIOV)
            ioCtx->rtree->UpdateValueWrapper(ioCtx->key, ioCtx->lba,
                                             sizeof(uint64_t));
        if (ioCtx->clb)
            ioCtx->clb(nullptr, StatusCode::OK, ioCtx->key, ioCtx->keySize,
                       ioCtx->buff, ioCtx->size);
    } else {
        if (ioCtx->clb)
            ioCtx->clb(nullptr, StatusCode::UNKNOWN_ERROR, ioCtx->key,
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
    OffloadIoCtx *ioCtx = reinterpret_cast<OffloadIoCtx *>(cb_arg);

    if (success) {
        if (ioCtx->clb)
            ioCtx->clb(nullptr, StatusCode::OK, ioCtx->key, ioCtx->keySize,
                       ioCtx->buff, ioCtx->size);
    } else {
        if (ioCtx->clb)
            ioCtx->clb(nullptr, StatusCode::UNKNOWN_ERROR, ioCtx->key,
                       ioCtx->keySize, nullptr, 0);
    }

    spdk_bdev_free_io(bdev_io);
}

OffloadPoller::OffloadPoller(RTreeEngine *rtree, SpdkCore *spdkCore,
                             const size_t cpuCore)
    : rtree(rtree), spdkCore(spdkCore), _cpuCore(cpuCore) {
    if (spdkCore->isOffloadEnabled()) {
        auto aligned =
            getBdev()->getAlignedSize(spdkCore->offloadOptions.allocUnitSize);
        _blkNumForLba = aligned / getBdevCtx()->blk_size;

        startThread();
    }
}

void OffloadPoller::startThread() {
    _thread = new std::thread(&OffloadPoller::_threadMain, this);
    DAQ_DEBUG("OffloadPoller thread started");
    if (_cpuCore) {
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(_cpuCore, &cpuset);

        const int set_result = pthread_setaffinity_np(
            _thread->native_handle(), sizeof(cpu_set_t), &cpuset);
        if (set_result == 0) {
            DAQ_DEBUG("OffloadPoller thread affinity set on CPU core [" +
                      std::to_string(_cpuCore) + "]");
        } else {
            DAQ_DEBUG("Cannot set affinity on CPU core [" +
                      std::to_string(_cpuCore) + "] for OffloadReactor");
        }
    }
}

bool OffloadPoller::read(OffloadIoCtx *ioCtx) {
    return spdk_bdev_read_blocks(getBdevDesc(), getBdevIoChannel(), ioCtx->buff,
                                 getBlockOffsetForLba(*ioCtx->lba),
                                 ioCtx->blockSize, read_complete, ioCtx);
}

bool OffloadPoller::write(OffloadIoCtx *ioCtx) {
    return spdk_bdev_write_blocks(getBdevDesc(), getBdevIoChannel(),
                                  ioCtx->buff,
                                  getBlockOffsetForLba(*ioCtx->lba),
                                  ioCtx->blockSize, write_complete, ioCtx);
}

void OffloadPoller::initFreeList() {
    auto initNeeded = false;
    if (getBdevCtx()) {
        if (boost::filesystem::exists(POOL_FREELIST_FILENAME)) {
            _offloadFreeList =
                pool<OffloadFreeList>::open(POOL_FREELIST_FILENAME, LAYOUT);
        } else {
            _offloadFreeList = pool<OffloadFreeList>::create(
                POOL_FREELIST_FILENAME, LAYOUT, POOL_FREELIST_SIZE,
                CREATE_MODE_RW);
            initNeeded = true;
        }
        freeLbaList = _offloadFreeList.get_root().get();
        freeLbaList->maxLba = getBdevCtx()->blk_num / _blkNumForLba;
        if (initNeeded) {
            freeLbaList->push(_offloadFreeList, -1);
        }
    }
}

StatusCode OffloadPoller::_getValCtx(const OffloadRqst *rqst,
                                     ValCtx &valCtx) const {
    return rtree->Get(rqst->key, rqst->keySize, &valCtx.val, &valCtx.size,
                      &valCtx.location);
}

void OffloadPoller::_processGet(const OffloadRqst *rqst) {
    ValCtx valCtx;

    auto rc = _getValCtx(rqst, valCtx);
    if (rc != StatusCode::OK) {
        _rqstClb(rqst, rc);
        return;
    }

    if (isValInPmem(valCtx)) {
        _rqstClb(rqst, StatusCode::KEY_NOT_FOUND);
        return;
    }

    auto size = getBdev()->getAlignedSize(valCtx.size);

    char *buff = reinterpret_cast<char *>(
        spdk_dma_zmalloc(size, getBdevCtx()->buf_align, NULL));

    auto blkSize = getBdev()->getSizeInBlk(size);
    OffloadIoCtx *ioCtx = new OffloadIoCtx{
        buff,      valCtx.size,   blkSize,
        rqst->key, rqst->keySize, static_cast<uint64_t *>(valCtx.val),
        false,     rtree,         rqst->clb};

    if (read(ioCtx) != 0)
        _rqstClb(rqst, StatusCode::UNKNOWN_ERROR);
}

void OffloadPoller::_processUpdate(const OffloadRqst *rqst) {
    OffloadIoCtx *ioCtx = nullptr;
    ValCtx valCtx;

    auto rc = _getValCtx(rqst, valCtx);
    if (rc != StatusCode::OK) {
        _rqstClb(rqst, rc);
        return;
    }

    if (isValInPmem(valCtx)) {

        const char *val;
        size_t valSize = 0;
        if (rqst->valueSize > 0) {
            val = rqst->value;
            valSize = rqst->valueSize;
        } else {
            val = static_cast<char *>(valCtx.val);
            valSize = valCtx.size;
        }

        auto valSizeAlign = getBdev()->getAlignedSize(valSize);
        auto buff = reinterpret_cast<char *>(
            spdk_dma_zmalloc(valSizeAlign, getBdevCtx()->buf_align, NULL));

        memcpy(buff, val, valSize);
        ioCtx = new OffloadIoCtx{
            buff,      valSize,       getBdev()->getSizeInBlk(valSizeAlign),
            rqst->key, rqst->keySize, nullptr,
            true,      rtree,         rqst->clb};
        rc = rtree->AllocateIOVForKey(rqst->key, &ioCtx->lba, sizeof(uint64_t));
        if (rc != StatusCode::OK) {
            delete ioCtx;
            _rqstClb(rqst, rc);
            return;
        }
        *ioCtx->lba = getFreeLba();

    } else if (isValOffloaded(valCtx)) {
        if (valCtx.size == 0) {
            _rqstClb(rqst, StatusCode::OK);
            return;
        }
        auto valSizeAlign = getBdev()->getAlignedSize(rqst->valueSize);
        auto buff = reinterpret_cast<char *>(
            spdk_dma_zmalloc(valSizeAlign, getBdevCtx()->buf_align, NULL));
        memcpy(buff, rqst->value, rqst->valueSize);

        ioCtx = new OffloadIoCtx{
            buff,      rqst->valueSize, getBdev()->getSizeInBlk(valSizeAlign),
            rqst->key, rqst->keySize,   new uint64_t,
            false,     rtree,           rqst->clb};
        *ioCtx->lba = *(static_cast<uint64_t *>(valCtx.val));

    } else {
        _rqstClb(rqst, StatusCode::KEY_NOT_FOUND);
        return;
    }

    if (write(ioCtx) != 0)
        _rqstClb(rqst, StatusCode::UNKNOWN_ERROR);
}

void OffloadPoller::_processRemove(const OffloadRqst *rqst) {

    ValCtx valCtx;

    auto rc = _getValCtx(rqst, valCtx);
    if (rc != StatusCode::OK) {
        _rqstClb(rqst, rc);
        return;
    }

    if (isValInPmem(valCtx)) {
        _rqstClb(rqst, StatusCode::KEY_NOT_FOUND);
        return;
    }

    uint64_t lba = *(static_cast<uint64_t *>(valCtx.val));

    freeLbaList->push(_offloadFreeList, lba);
    rtree->Remove(rqst->key);
    _rqstClb(rqst, StatusCode::OK);
}

void OffloadPoller::process() {
    if (requestCount > 0) {
        for (unsigned short RqstIdx = 0; RqstIdx < requestCount; RqstIdx++) {
            OffloadRqst *rqst = requests[RqstIdx];
            switch (rqst->op) {
            case OffloadOperation::GET:
                _processGet(const_cast<const OffloadRqst *>(rqst));
                break;
            case OffloadOperation::UPDATE: {
                _processUpdate(const_cast<const OffloadRqst *>(rqst));
                break;
            }
            case OffloadOperation::REMOVE: {
                _processRemove(const_cast<const OffloadRqst *>(rqst));
                break;
            }
            default:
                break;
            }
            delete requests[RqstIdx];
        }
        requestCount = 0;
    }
}

void OffloadPoller::_threadMain(void) {
    while (true) {
        dequeue();
        process();
        spdkCore->spSpdkThread->poll();
    }
}

int64_t OffloadPoller::getFreeLba() {
    return freeLbaList->get(_offloadFreeList);
}

} // namespace DaqDB
