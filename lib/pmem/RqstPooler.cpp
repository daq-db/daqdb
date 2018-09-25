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

#include "spdk/env.h"

#include "RqstPooler.h"
#include "OffloadPooler.h"
#include <Logger.h>

namespace DaqDB {

RqstPooler::RqstPooler(std::shared_ptr<DaqDB::RTreeEngine> &rtree,
                       const size_t cpuCore)
    : isRunning(0), _thread(nullptr), rtree(rtree), _cpuCore(cpuCore),
      requests(OFFLOAD_DEQUEUE_RING_LIMIT, 0) {

    rqstRing = spdk_ring_create(SPDK_RING_TYPE_MP_SC, 4096 * 4,
                                SPDK_ENV_SOCKET_ID_ANY);

    StartThread();
}

RqstPooler::~RqstPooler() {
    spdk_ring_free(rqstRing);
    isRunning = 0;
    if (_thread != nullptr)
        _thread->join();
}

void RqstPooler::StartThread() {
    isRunning = 1;
    _thread = new std::thread(&RqstPooler::_ThreadMain, this);

    if (_cpuCore) {
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(_cpuCore, &cpuset);

        // @TODO jradtke: Should be replaced with spdk_env_thread_launch_pinned
        const int set_result = pthread_setaffinity_np(
            _thread->native_handle(), sizeof(cpu_set_t), &cpuset);
        if (set_result == 0) {
            FOG_DEBUG("Started RqstPooler on CPU core [" +
                      std::to_string(_cpuCore) + "]");
        } else {
            FOG_DEBUG("Cannot set affinity on CPU core [" +
                      std::to_string(_cpuCore) + "]");
        }
    }
}

void RqstPooler::_ThreadMain() {
    while (isRunning) {
        Dequeue();
        Process();
    }
}

bool RqstPooler::Enqueue(PmemRqst *Message) {
    size_t count = spdk_ring_enqueue(rqstRing, (void **)&Message, 1);
    return (count == 1);
}

void RqstPooler::Dequeue() {
    requestCount =
        spdk_ring_dequeue(rqstRing, (void **)&requests[0], DEQUEUE_RING_LIMIT);
    assert(requestCount <= DEQUEUE_RING_LIMIT);
}

void RqstPooler::_ProcessTransfer(const PmemRqst *rqst) {
    if (!offloadPooler) {
        FOG_DEBUG("Request transfer failed. Offload pooler not set");
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

void RqstPooler::_ProcessGet(const PmemRqst *rqst) {
    ValCtx valCtx;
    StatusCode rc = rtree->Get(rqst->key, rqst->keySize, &valCtx.val,
                               &valCtx.size, &valCtx.location);

    if (rc != StatusCode::Ok || !valCtx.val) {
        _RqstClb(rqst, rc);
        return;
    }

    if (_ValOffloaded(valCtx)) {
        _ProcessTransfer(rqst);
    }

    Value value(new char[valCtx.size], valCtx.size);
    std::memcpy(value.data(), valCtx.val, valCtx.size);
    _RqstClb(rqst, rc, value);
}
void RqstPooler::_ProcessPut(const PmemRqst *rqst) {
    StatusCode rc =
        rtree->Put(rqst->key, rqst->keySize, rqst->value, rqst->valueSize);
    _RqstClb(rqst, rc);
}

void RqstPooler::Process() {
    for (unsigned short RqstIdx = 0; RqstIdx < requestCount; RqstIdx++) {
        PmemRqst* rqst = requests[RqstIdx];

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
