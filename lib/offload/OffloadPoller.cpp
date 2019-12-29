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
#include <RTreeEngine.h>
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
        freeLbaList->maxLba = getBdevCtx()->blk_num / getBdev()->blkNumForLba;
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
        OffloadRqst::getPool.put(rqst);
        return;
    }

    if (valCtx.location == LOCATIONS::PMEM) {
        _rqstClb(rqst, StatusCode::KEY_NOT_FOUND);
        OffloadRqst::getPool.put(rqst);
        return;
    }

    SpdkDevice *spdkDev = getBdev();

    rqst->valueSize = valCtx.size;
    DeviceTask *ioTask =
        new (rqst->taskBuffer) DeviceTask{0,
                                          spdkDev->getOptimalSize(valCtx.size),
                                          0,
                                          rqst->keySize,
                                          static_cast<DeviceAddr *>(valCtx.val),
                                          false,
                                          rtree,
                                          rqst->clb,
                                          spdkDev,
                                          rqst,
                                          OffloadOperation::GET};
    memcpy(ioTask->key, rqst->key, rqst->keySize);

    if (spdkDev->read(ioTask) != true) {
        _rqstClb(rqst, StatusCode::UNKNOWN_ERROR);
        OffloadRqst::getPool.put(rqst);
    }
}

void OffloadPoller::_processUpdate(OffloadRqst *rqst) {
    DeviceTask *ioTask = nullptr;

    if (rqst == nullptr) {
        _rqstClb(rqst, StatusCode::UNKNOWN_ERROR);
        return;
    }

    SpdkDevice *spdkDev = getBdev();
    auto valSizeAlign = spdkDev->getAlignedSize(rqst->valueSize);
    if (rqst->loc == LOCATIONS::PMEM) {
        ioTask = new (rqst->taskBuffer)
            DeviceTask{0,
                       spdkDev->getOptimalSize(rqst->valueSize),
                       spdkDev->getSizeInBlk(valSizeAlign),
                       rqst->keySize,
                       nullptr,
                       true,
                       rtree,
                       rqst->clb,
                       spdkDev,
                       rqst,
                       OffloadOperation::UPDATE};
        memcpy(ioTask->key, rqst->key, rqst->keySize);
        ioTask->freeLba = getFreeLba();
    } else if (rqst->loc == LOCATIONS::DISK) {
        if (rqst->valueSize == 0) {
            _rqstClb(rqst, StatusCode::OK);
            OffloadRqst::updatePool.put(rqst);
            return;
        }

        ioTask = new (rqst->taskBuffer)
            DeviceTask{0,
                       spdkDev->getOptimalSize(rqst->valueSize),
                       spdkDev->getSizeInBlk(valSizeAlign),
                       rqst->keySize,
                       new (rqst->devAddrBuf) DeviceAddr,
                       false,
                       rtree,
                       rqst->clb,
                       spdkDev,
                       rqst,
                       OffloadOperation::UPDATE};
        memcpy(ioTask->key, rqst->key, rqst->keySize);
        memcpy(ioTask->bdevAddr, rqst->value, sizeof(*ioTask->bdevAddr));
    } else {
        _rqstClb(rqst, StatusCode::KEY_NOT_FOUND);
        OffloadRqst::updatePool.put(rqst);
        return;
    }

    if (spdkDev->write(ioTask) != true) {
        _rqstClb(rqst, StatusCode::UNKNOWN_ERROR);
        OffloadRqst::updatePool.put(rqst);
    }
}

void OffloadPoller::_processRemove(OffloadRqst *rqst) {

    ValCtx valCtx;

    auto rc = _getValCtx(rqst, valCtx);
    if (rc != StatusCode::OK) {
        _rqstClb(rqst, rc);
        if (rqst->valueSize > 0)
            delete[] rqst->value;
        OffloadRqst::removePool.put(rqst);
        return;
    }

    if (valCtx.location == LOCATIONS::PMEM) {
        _rqstClb(rqst, StatusCode::KEY_NOT_FOUND);
        if (rqst->valueSize > 0)
            delete[] rqst->value;
        OffloadRqst::removePool.put(rqst);
        return;
    }

    uint64_t lba = *(static_cast<uint64_t *>(valCtx.val));

    freeLbaList->push(_offloadFreeList, lba);
    rtree->Remove(rqst->key);
    _rqstClb(rqst, StatusCode::OK);
    OffloadRqst::removePool.put(rqst);
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
