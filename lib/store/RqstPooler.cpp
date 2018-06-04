/**
 * Copyright 2018, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of the copyright holder nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "RqstPooler.h"

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
            FOG_DEBUG("Started RqstPooler on CPU core [" + std::to_string(_cpuCore) + "]");
        } else {
            FOG_DEBUG("Cannot set affinity on CPU core [" + std::to_string(_cpuCore) + "]");
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
	for (unsigned short MsgIndex = 0; MsgIndex < _dequedCount; MsgIndex++) {
		std::string keyStr(_rqstMsgBuffer[MsgIndex]->key->data(),
				_rqstMsgBuffer[MsgIndex]->key->size());

		switch (_rqstMsgBuffer[MsgIndex]->op) {
		case RqstOperation::PUT: {
			/** @TODO jradtke: Not thread safe */
			std::string valStr(_rqstMsgBuffer[MsgIndex]->value->data(),
					_rqstMsgBuffer[MsgIndex]->value->size());
			mRTree->Put(keyStr, valStr);
			break;
		}
			break;
		}
               case RqstOperation::PUTDISK: {
                       unsigned int cluster = 0;
                       std::string valStr(_rqstMsgBuffer[MsgIndex]->value->data(),
                                       _rqstMsgBuffer[MsgIndex]->value->size());
                       /** @TODO: ppelplin: get free cluster for data */
                       /*              cluster = get_free_cluster(); */
                       /** @TODO: ppelplin: send i/o */

//                     mDisk->Store(valStr, cluster, [&] {
                           /** @TODO: ppelplin: Update metadata in cb:
                            * value points to the used cluster
                            * location points to disk */
                           /*mRTree->Update(keyStr, cluster, disk);*/
//                     });
		case RqstOperation::GET:
			break;
		case RqstOperation::UPDATE:
			break;
		case RqstOperation::DELETE:
			break;
		default:
			break;
		}

		(*_rqstMsgBuffer[0]->cb_fn)(nullptr, FogKV::Status(FogKV::Code::Ok),
				*_rqstMsgBuffer[0]->key, *_rqstMsgBuffer[0]->value);
	}
}

} /* namespace FogKV */
