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

#include "PmemPoller.h"

#include <iostream>
#include <pthread.h>
#include <string>

#include "spdk/env.h"

#include "../offload/OffloadPoller.h"
#include <Logger.h>

namespace DaqDB {

PmemPoller::PmemPoller(RTreeEngine *rtree, const size_t cpuCore)
    : isRunning(0), _thread(nullptr), rtree(rtree), _cpuCore(cpuCore) {
    startThread();
}

PmemPoller::~PmemPoller() {
    isRunning = 0;
    if (_thread != nullptr)
        _thread->join();
}

void PmemPoller::startThread() {
    isRunning = 1;
    _thread = new std::thread(&PmemPoller::_threadMain, this);

    if (_cpuCore) {
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(_cpuCore, &cpuset);

        // @TODO jradtke: Should be replaced with spdk_env_thread_launch_pinned
        const int set_result = pthread_setaffinity_np(
            _thread->native_handle(), sizeof(cpu_set_t), &cpuset);
        if (set_result == 0) {
            DAQ_DEBUG("Started RqstPoller on CPU core [" +
                      std::to_string(_cpuCore) + "]");
        } else {
            DAQ_DEBUG("Cannot set affinity on CPU core [" +
                      std::to_string(_cpuCore) + "]");
        }
    }
}

void PmemPoller::_threadMain() {
    while (isRunning) {
        dequeue();
        process();
    }
}

void PmemPoller::_processTransfer(const PmemRqst *rqst) {
    if (!offloadPoller) {
        DAQ_DEBUG("Request transfer failed. Offload poller not set");
        _rqstClb(rqst, StatusCode::OFFLOAD_DISABLED_ERROR);
    }
    try {
        OffloadRqst *getRqst = OffloadRqst::getPool.get();
        getRqst->finalizeGet(rqst->key, rqst->keySize, nullptr, 0, rqst->clb);

        if (!offloadPoller->enqueue(getRqst)) {
            OffloadRqst::getPool.put(getRqst);
            _rqstClb(rqst, StatusCode::UNKNOWN_ERROR);
        }
    } catch (OperationFailedException &e) {
        _rqstClb(rqst, StatusCode::UNKNOWN_ERROR);
    }
}

void PmemPoller::_processGet(const PmemRqst *rqst) {
    StatusCode rc = StatusCode::OK;
    ValCtx valCtx;
    try {
        rtree->Get(rqst->key, rqst->keySize, &valCtx.val, &valCtx.size,
                   &valCtx.location);
    } catch (...) {
        /** @todo fix exception handling */
        rc = StatusCode::UNKNOWN_ERROR;
    }

    if (!valCtx.val) {
        _rqstClb(rqst, rc);
        return;
    }

    if (_valOffloaded(valCtx)) {
        _processTransfer(rqst);
        return;
    }

    Value value(new char[valCtx.size], valCtx.size);
    std::memcpy(value.data(), valCtx.val, valCtx.size);
    _rqstClb(rqst, StatusCode::OK, value);
}
void PmemPoller::_processPut(const PmemRqst *rqst) {
    StatusCode rc = StatusCode::OK;
    try {
        rtree->Put(rqst->key, rqst->keySize, rqst->value, rqst->valueSize);
    } catch (...) {
        /** @todo fix exception handling */
        rc = StatusCode::UNKNOWN_ERROR;
    }
    _rqstClb(rqst, rc);
}

void PmemPoller::process() {
    if (requestCount > 0) {
        for (unsigned short RqstIdx = 0; RqstIdx < requestCount; RqstIdx++) {
            PmemRqst *rqst = requests[RqstIdx];

            switch (rqst->op) {
            case RqstOperation::PUT:
                _processPut(rqst);
                break;
            case RqstOperation::GET: {
                _processGet(rqst);
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
} // namespace DaqDB
