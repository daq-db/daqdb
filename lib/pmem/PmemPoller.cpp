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
        if (!offloadPoller->enqueue(new OffloadRqst(OffloadOperation::GET,
                                                    rqst->key, rqst->keySize,
                                                    nullptr, 0, rqst->clb))) {
            _rqstClb(rqst, StatusCode::UNKNOWN_ERROR);
        }
    } catch (OperationFailedException &e) {
        _rqstClb(rqst, StatusCode::UNKNOWN_ERROR);
    }
}

void PmemPoller::_processGet(const PmemRqst *rqst) {
    ValCtx valCtx;
    StatusCode rc = rtree->Get(rqst->key, rqst->keySize, &valCtx.val,
                               &valCtx.size, &valCtx.location);

    if (rc != StatusCode::OK || !valCtx.val) {
        _rqstClb(rqst, rc);
        return;
    }

    if (_valOffloaded(valCtx)) {
        _processTransfer(rqst);
        return;
    }

    Value value(new char[valCtx.size], valCtx.size);
    std::memcpy(value.data(), valCtx.val, valCtx.size);
    _rqstClb(rqst, rc, value);
}
void PmemPoller::_processPut(const PmemRqst *rqst) {
    StatusCode rc =
        rtree->Put(rqst->key, rqst->keySize, rqst->value, rqst->valueSize);
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
