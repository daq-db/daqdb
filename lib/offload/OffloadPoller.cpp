/**
 *  Copyright (c) 2019 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License. 
 */

#include <stdio.h>
#include <time.h>

#include <iostream>
#include <pthread.h>
#include <string>
#include <thread>

#include <boost/asio.hpp>
#include <boost/filesystem.hpp>

#include "spdk/stdinc.h"
#include "spdk/cpuset.h"
#include "spdk/queue.h"
#include "spdk/log.h"
#include "spdk/thread.h"
#include "spdk/event.h"
#include "spdk/ftl.h"
#include "spdk/conf.h"
#include "spdk/env.h"

#include "OffloadPoller.h"
#include <Logger.h>
#include <RTree.h>
#include <daqdb/Status.h>


#define POOL_FREELIST_SIZE 1ULL * 1024 * 1024 * 1024
#define LAYOUT "queue"
#define CREATE_MODE_RW (S_IWUSR | S_IRUSR)

namespace DaqDB {

const char *OffloadPoller::pmemFreeListFilename = "/mnt/pmem/offload_free.pm";

OffloadPoller::OffloadPoller(RTreeEngine *rtree, SpdkCore *_spdkCore)
    : Poller<OffloadRqst>(false), rtree(rtree), spdkCore(_spdkCore) {}

OffloadPoller::~OffloadPoller() {
    isRunning = 0;
}

void OffloadPoller::initFreeList() {
    auto initNeeded = false;
    if (getBdevCtx()) {
        if (boost::filesystem::exists(OffloadPoller::pmemFreeListFilename)) {
            _offloadFreeList =
                pool<OffloadFreeList>::open(OffloadPoller::pmemFreeListFilename, LAYOUT);
        } else {
            _offloadFreeList = pool<OffloadFreeList>::create(
                    OffloadPoller::pmemFreeListFilename, LAYOUT, POOL_FREELIST_SIZE,
                CREATE_MODE_RW);
            initNeeded = true;
        }
        freeLbaList = _offloadFreeList.get_root().get();
        freeLbaList->maxLba = getBdevCtx()->blk_num / getBdev()->_blkNumForLba;
        if (initNeeded) {
            freeLbaList->push(_offloadFreeList, -1);
        }
    }
}

StatusCode OffloadPoller::_getValCtx(const OffloadRqst *rqst,
                                     ValCtx &valCtx) const {
    try {
        rtree->Get(rqst->key, rqst->keySize, &valCtx.val, &valCtx.size,
                   &valCtx.location);
    } catch (...) {
        /** @todo fix exception handling */
        return StatusCode::UNKNOWN_ERROR;
    }
    return StatusCode::OK;
}

void OffloadPoller::_processGet(OffloadRqst *rqst) {
    ValCtx valCtx;

    auto rc = _getValCtx(rqst, valCtx);
    if (rc != StatusCode::OK) {
        _rqstClb(rqst, rc);
        delete[] rqst->key;
        delete rqst;
        return;
    }

    if (isValInPmem(valCtx)) {
        _rqstClb(rqst, StatusCode::KEY_NOT_FOUND);
        delete[] rqst->key;
        delete rqst;
        return;
    }

    auto size = getBdev()->getAlignedSize(valCtx.size);

    char *buff = reinterpret_cast<char *>(
        spdk_dma_zmalloc(size, getBdevCtx()->buf_align, NULL));
    getBdev()->IoBytesQueued += size;

    auto blkSize = getBdev()->getSizeInBlk(size);
    DeviceTask *ioTask = new (rqst->taskBuffer)
        DeviceTask{buff,
                   getBdev()->getOptimalSize(valCtx.size),
                   blkSize,
                   rqst->keySize,
                   static_cast<DeviceAddr *>(valCtx.val),
                   false,
                   rtree,
                   rqst->clb,
                   getBdev(),
                   rqst,
                   OffloadOperation::GET};
    memcpy(ioTask->key, rqst->key, rqst->keySize);
    delete[] rqst->key;

    if (getBdev()->read(ioTask) != true)
        _rqstClb(rqst, StatusCode::UNKNOWN_ERROR);
}

void OffloadPoller::_processUpdate(OffloadRqst *rqst) {
    DeviceTask *ioTask = nullptr;
    ValCtx valCtx;

    if (rqst == nullptr) {
        _rqstClb(rqst, StatusCode::UNKNOWN_ERROR);
        delete[] rqst->key;
        if (rqst->valueSize > 0)
            delete[] rqst->value;
        delete rqst;
        return;
    }

    auto rc = _getValCtx(rqst, valCtx);
    if (rc != StatusCode::OK) {
        _rqstClb(rqst, rc);
        delete[] rqst->key;
        if (rqst->valueSize > 0)
            delete[] rqst->value;
        delete rqst;
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
        getBdev()->IoBytesQueued += getBdev()->getOptimalSize(valSize);

        memcpy(buff, val, valSize);
        ioTask = new (rqst->taskBuffer)
            DeviceTask{buff,
                       getBdev()->getOptimalSize(valSize),
                       getBdev()->getSizeInBlk(valSizeAlign),
                       rqst->keySize,
                       nullptr,
                       true,
                       rtree,
                       rqst->clb,
                       getBdev(),
                       rqst,
                       OffloadOperation::UPDATE};
        memcpy(ioTask->key, rqst->key, rqst->keySize);

        try {
            rtree->AllocateIOVForKey(
                rqst->key, reinterpret_cast<uint64_t **>(&ioTask->bdevAddr),
                sizeof(DeviceAddr));
        } catch (...) {
            /** @todo fix exception handling */
            _rqstClb(rqst, StatusCode::UNKNOWN_ERROR);
            delete[] rqst->key;
            if (rqst->valueSize > 0)
                delete[] rqst->value;
            delete rqst;
            return;
        }
        ioTask->bdevAddr->lba = getFreeLba();
    } else if (isValOffloaded(valCtx)) {
        if (valCtx.size == 0) {
            _rqstClb(rqst, StatusCode::OK);
            delete[] rqst->key;
            if (rqst->valueSize > 0)
                delete[] rqst->value;
            delete rqst;
            return;
        }
        auto valSizeAlign = getBdev()->getAlignedSize(rqst->valueSize);
        auto buff = reinterpret_cast<char *>(
            spdk_dma_zmalloc(valSizeAlign, getBdevCtx()->buf_align, NULL));
        getBdev()->IoBytesQueued += getBdev()->getOptimalSize(rqst->valueSize);

        memcpy(buff, rqst->value, rqst->valueSize);

        ioTask = new (rqst->taskBuffer)
            DeviceTask{buff,
                       getBdev()->getOptimalSize(rqst->valueSize),
                       getBdev()->getSizeInBlk(valSizeAlign),
                       rqst->keySize,
                       new DeviceAddr,
                       false,
                       rtree,
                       rqst->clb,
                       getBdev(),
                       rqst,
                       OffloadOperation::UPDATE};
        memcpy(ioTask->key, rqst->key, rqst->keySize);
        ioTask->bdevAddr->lba = *(static_cast<uint64_t *>(valCtx.val));
    } else {
        _rqstClb(rqst, StatusCode::KEY_NOT_FOUND);
        delete[] rqst->key;
        if (rqst->valueSize > 0)
            delete[] rqst->value;
        delete rqst;
        return;
    }

    delete[] rqst->key;
    if (rqst->valueSize > 0)
        delete[] rqst->value;

    if (getBdev()->write(ioTask) != true)
        _rqstClb(rqst, StatusCode::UNKNOWN_ERROR);
}

void OffloadPoller::_processRemove(OffloadRqst *rqst) {

    ValCtx valCtx;

    auto rc = _getValCtx(rqst, valCtx);
    if (rc != StatusCode::OK) {
        _rqstClb(rqst, rc);
        delete rqst;
        return;
    }

    if (isValInPmem(valCtx)) {
        _rqstClb(rqst, StatusCode::KEY_NOT_FOUND);
        delete rqst;
        return;
    }

    uint64_t lba = *(static_cast<uint64_t *>(valCtx.val));

    freeLbaList->push(_offloadFreeList, lba);
    rtree->Remove(rqst->key);
    _rqstClb(rqst, StatusCode::OK);
    delete[] rqst->key;
    delete rqst;
}

void OffloadPoller::process() {
    if (requestCount > 0) {
        for (unsigned short RqstIdx = 0; RqstIdx < requestCount; RqstIdx++) {
            OffloadRqst *rqst = requests[RqstIdx];
            switch (rqst->op) {
            case OffloadOperation::GET:
                _processGet(rqst);
                break;
            case OffloadOperation::UPDATE: {
                _processUpdate(rqst);
                break;
            }
            case OffloadOperation::REMOVE: {
                _processRemove(rqst);
                break;
            }
            default:
                break;
            }
        }
        requestCount = 0;
    }
}

int64_t OffloadPoller::getFreeLba() {
    return freeLbaList->get(_offloadFreeList);
}

} // namespace DaqDB
