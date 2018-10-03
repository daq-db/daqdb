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

#include "PmemPooler.h"

#include <iostream>
#include <pthread.h>
#include <string>

#include "spdk/env.h"

#include "OffloadPooler.h"
#include <Logger.h>

namespace DaqDB {

PmemPooler::PmemPooler(std::shared_ptr<DaqDB::RTreeEngine> &rtree,
                       const size_t cpuCore)
    : isRunning(0), _thread(nullptr), rtree(rtree), _cpuCore(cpuCore) {
    StartThread();
}

PmemPooler::~PmemPooler() {
    isRunning = 0;
    if (_thread != nullptr)
        _thread->join();
}

void PmemPooler::StartThread() {
    isRunning = 1;
    _thread = new std::thread(&PmemPooler::_ThreadMain, this);

    if (_cpuCore) {
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(_cpuCore, &cpuset);

        // @TODO jradtke: Should be replaced with spdk_env_thread_launch_pinned
        const int set_result = pthread_setaffinity_np(
            _thread->native_handle(), sizeof(cpu_set_t), &cpuset);
        if (set_result == 0) {
            DAQ_DEBUG("Started RqstPooler on CPU core [" +
                      std::to_string(_cpuCore) + "]");
        } else {
            DAQ_DEBUG("Cannot set affinity on CPU core [" +
                      std::to_string(_cpuCore) + "]");
        }
    }
}

void PmemPooler::_ThreadMain() {
    while (isRunning) {
        Dequeue();
        Process();
    }
}

void PmemPooler::_ProcessTransfer(const PmemRqst *rqst) {
    if (!offloadPooler) {
        DAQ_DEBUG("Request transfer failed. Offload pooler not set");
        _RqstClb(rqst, StatusCode::OffloadDisabledError);
    }
    try {
        if (!offloadPooler->Enqueue(new OffloadRqst(OffloadOperation::GET,
                                                    rqst->key, rqst->keySize,
                                                    nullptr, 0, rqst->clb))) {
            _RqstClb(rqst, StatusCode::UnknownError);
        }
    } catch (OperationFailedException &e) {
        _RqstClb(rqst, StatusCode::UnknownError);
    }
}

void PmemPooler::_ProcessGet(const PmemRqst *rqst) {
    ValCtx valCtx;
    StatusCode rc = rtree->Get(rqst->key, rqst->keySize, &valCtx.val,
                               &valCtx.size, &valCtx.location);

    if (rc != StatusCode::Ok || !valCtx.val) {
        _RqstClb(rqst, rc);
        return;
    }

    if (_ValOffloaded(valCtx)) {
        _ProcessTransfer(rqst);
        return;
    }

    Value value(new char[valCtx.size], valCtx.size);
    std::memcpy(value.data(), valCtx.val, valCtx.size);
    _RqstClb(rqst, rc, value);
}
void PmemPooler::_ProcessPut(const PmemRqst *rqst) {
    StatusCode rc =
        rtree->Put(rqst->key, rqst->keySize, rqst->value, rqst->valueSize);
    _RqstClb(rqst, rc);
}

void PmemPooler::Process() {
    if (requestCount > 0) {
        for (unsigned short RqstIdx = 0; RqstIdx < requestCount; RqstIdx++) {
            PmemRqst *rqst = requests[RqstIdx];

            switch (rqst->op) {
            case RqstOperation::PUT:
                _ProcessPut(rqst);
                break;
            case RqstOperation::GET: {
                _ProcessGet(rqst);
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
