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

#include <atomic>
#include <cstdint>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>

#include "spdk/conf.h"
#include "spdk/cpuset.h"
#include "spdk/env.h"
#include "spdk/event.h"
#include "spdk/ftl.h"
#include "spdk/log.h"
#include "spdk/queue.h"
#include "spdk/stdinc.h"
#include "spdk/thread.h"

#include "FinalizePoller.h"

namespace DaqDB {

FinalizePoller::FinalizePoller() : FinPoller() {}

void FinalizePoller::process() {
    if (requestCount > 0) {
        for (unsigned short RqstIdx = 0; RqstIdx < requestCount; RqstIdx++) {
            DeviceTask *task = requests[RqstIdx];
            if (!task) // due to possible timeout
                continue;
            bool dropIt = false;
            if (_state != FinalizePoller::State::FP_READY)
                dropIt = true;

            switch (task->op) {
            case OffloadOperation::GET:
                if (dropIt == true)
                    OffloadRqst::getPool.put(task->rqst);
                else
                    _processGet(task);
                break;
            case OffloadOperation::UPDATE:
                if (dropIt == true)
                    OffloadRqst::updatePool.put(task->rqst);
                else
                    _processUpdate(task);
                break;
            case OffloadOperation::REMOVE:
                if (dropIt == true)
                    OffloadRqst::removePool.put(task->rqst);
                else
                    _processRemove(task);
                break;
            default:
                break;
            }
        }
        requestCount = 0;
    } else {
        if (_state == FinalizePoller::State::FP_QUIESCENT)
            _state = FinalizePoller::State::FP_QUIESCENT;
    }
}

void FinalizePoller::_processGet(DeviceTask *task) {
    SpdkBdev *bdev = reinterpret_cast<SpdkBdev *>(task->bdev);

    if (task->result) {
        if (task->clb)
            task->clb(nullptr, StatusCode::OK, task->key, task->keySize,
                      task->buff->getSpdkDmaBuf(), task->rqst->valueSize);
    } else {
        if (task->clb)
            task->clb(nullptr, StatusCode::UNKNOWN_ERROR, task->key,
                      task->keySize, nullptr, 0);
    }

    bdev->ioPoolMgr->putIoReadBuf(task->buff);
    bdev->ioBufsInUse--;
    OffloadRqst::getPool.put(task->rqst);
}

void FinalizePoller::_processUpdate(DeviceTask *task) {
    SpdkBdev *bdev = reinterpret_cast<SpdkBdev *>(task->bdev);

    try {
        task->rtree->AllocateIOVForKey(task->key, &task->bdevAddr,
                                       sizeof(DeviceAddr));
    } catch (...) {
        if (task->clb)
            task->clb(nullptr, StatusCode::UNKNOWN_ERROR, task->key,
                      task->keySize, nullptr, 0);
        OffloadRqst::updatePool.put(task->rqst);
        return;
    }
    task->bdevAddr->busAddr.pciAddr = bdev->spBdevCtx.pci_addr;
    task->bdevAddr->lba = task->freeLba;

    if (task->result) {
        if (task->updatePmemIOV)
            task->rtree->UpdateValueWrapper(task->key, task->bdevAddr,
                                            sizeof(DeviceAddr));
        if (task->clb)
            task->clb(nullptr, StatusCode::OK, task->key, task->keySize,
                      task->buff->getSpdkDmaBuf(), task->rqst->valueSize);
    } else {
        if (task->clb)
            task->clb(nullptr, StatusCode::UNKNOWN_ERROR, task->key,
                      task->keySize, nullptr, 0);
    }

    OffloadRqst::updatePool.put(task->rqst);
}

void FinalizePoller::_processRemove(DeviceTask *task) {
    SpdkBdev *bdev = reinterpret_cast<SpdkBdev *>(task->bdev);

    if (task->result) {
        task->rtree->Remove(task->rqst->key);
        if (task->clb)
            task->clb(nullptr, StatusCode::OK, task->key, task->keySize,
                      nullptr, 0);
    } else {
        if (task->clb)
            task->clb(nullptr, StatusCode::UNKNOWN_ERROR, task->key,
                      task->keySize, nullptr, 0);
    }

    OffloadRqst::removePool.put(task->rqst);
}

} // namespace DaqDB
