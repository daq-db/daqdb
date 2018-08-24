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

#include "RqstPooler.h"
#include "OffloadRqstPooler.h"

#include <iostream>
#include <pthread.h>
#include <string>

#include "../debug/Logger.h"
#include "spdk/env.h"

namespace FogKV {

RqstMsg::RqstMsg(const RqstOperation op, const char *key, const size_t keySize,
                 const char *value, size_t valueSize,
                 KVStoreBase::KVStoreBaseCallback clb)
    : op(op), key(key), keySize(keySize), value(value), valueSize(valueSize),
      clb(clb) {}

RqstPooler::RqstPooler(std::shared_ptr<FogKV::RTreeEngine> rtree,
                       const size_t cpuCore)
    : isRunning(0), _thread(nullptr), rtree(rtree), _cpuCore(cpuCore) {

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
        DequeueMsg();
        ProcessMsg();
    }
}

bool RqstPooler::EnqueueMsg(RqstMsg *Message) {
    size_t count = spdk_ring_enqueue(rqstRing, (void **)&Message, 1);
    return (count == 1);
}

void RqstPooler::DequeueMsg() {
    processArrayCount =
        spdk_ring_dequeue(rqstRing, (void **)processArray, DEQUEUE_RING_LIMIT);
    assert(processArrayCount <= DEQUEUE_RING_LIMIT);
}

void RqstPooler::ProcessMsg() {
    for (unsigned short MsgIndex = 0; MsgIndex < processArrayCount;
         MsgIndex++) {
        const char *key = processArray[MsgIndex]->key;
        const size_t keySize = processArray[MsgIndex]->keySize;

        auto cb_fn = processArray[MsgIndex]->clb;

        switch (processArray[MsgIndex]->op) {
        case RqstOperation::PUT: {
            const char *val = processArray[MsgIndex]->value;
            const size_t valSize = processArray[MsgIndex]->valueSize;

            StatusCode rc = rtree->Put(key, keySize, val, valSize);
            if (cb_fn)
                cb_fn(nullptr, Status(rc), key, keySize, val, valSize);
            break;
        }
        case RqstOperation::GET: {
            size_t size;
            char *pVal;
            uint8_t location;

            StatusCode rc =
                rtree->Get(key, keySize, reinterpret_cast<void **>(&pVal),
                           &size, &location);
            if (rc != StatusCode::Ok || !pVal) {
                if (cb_fn)
                    cb_fn(nullptr, Status(rc), key, keySize, nullptr, 0);
                break;
            }

            if (location == DISK) {
                if (offloadPooler) {
                    try {
                        if (!offloadPooler->EnqueueMsg(new OffloadRqstMsg(
                                OffloadRqstOperation::GET, key, keySize,
                                nullptr, 0, cb_fn))) {
                            if (cb_fn)
                                cb_fn(nullptr, Status(StatusCode::UnknownError),
                                      key, keySize, nullptr, 0);
                        }
                    } catch (OperationFailedException &e) {
                        if (cb_fn)
                            cb_fn(nullptr, Status(StatusCode::UnknownError),
                                  key, keySize, nullptr, 0);
                    }
                } else {
                    if (cb_fn)
                        cb_fn(nullptr, Status(StatusCode::OffloadDisabledError),
                              key, keySize, nullptr, 0);
                }
            } else {
                Value value(new char[size], size);
                std::memcpy(value.data(), pVal, size);
                if (cb_fn)
                    cb_fn(nullptr, Status(rc), key, keySize, value.data(),
                          size);
            }
            break;
        }
        default:
            break;
        }
        delete processArray[MsgIndex];
    }
    processArrayCount = 0;
}

} /* namespace FogKV */
