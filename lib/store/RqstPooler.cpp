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

#include "spdk/env.h"

namespace FogKV {

RqstMsg::RqstMsg(RqstOperation op, Key *key, Value *value,
		KVStoreBase::KVStoreBasePutCallback *cb_fn) :
		op(op), key(key), value(value), cb_fn(cb_fn) {
}

RqstPooler::RqstPooler(std::shared_ptr<FogKV::RTreeEngine> Store) :
		isRunning(0), _thread(nullptr), mRTree(Store) {
	/** @TODO jradtke: ring size should be configurable? */
	submitRing = spdk_ring_create(SPDK_RING_TYPE_MP_SC, 4096 * 4,
	SPDK_ENV_SOCKET_ID_ANY);
	Start();
}

RqstPooler::~RqstPooler() {
	spdk_ring_free(submitRing);
	isRunning = 0;
	if (_thread != nullptr)
		_thread->join();
}

void RqstPooler::Start() {
	isRunning = 1;
	_thread = new std::thread(&RqstPooler::ThreadMain, this);
}

void RqstPooler::ThreadMain() {
	while (isRunning) {
		DequeueMsg();
		ProcessMsg();

		/** @TODO jradtke: Initial implementation, will be removed */
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
}

bool RqstPooler::EnqueueMsg(RqstMsg *Message) {
	size_t count = spdk_ring_enqueue(submitRing, (void **) &Message, 1);
	return (count == 1);

	/** @TODO jradtke: Initial implementation, error handling not implemented */
}

void RqstPooler::DequeueMsg() {
	_dequedCount = spdk_ring_dequeue(submitRing, (void **) _rqstMsgBuffer, DEQUEUE_RING_LIMIT);
	assert(_dequedCount < DEQUEUE_RING_LIMIT);
}

void RqstPooler::ProcessMsg() {
	for (unsigned short MsgIndex = 0; MsgIndex < _dequedCount; MsgIndex++) {
		std::string keyStr(_rqstMsgBuffer[MsgIndex]->key->data(),
				_rqstMsgBuffer[MsgIndex]->key->size());

		switch (_rqstMsgBuffer[MsgIndex]->op) {
		case RqstOperation::PUTPMEM: {
			/** @TODO jradtke: Not thread safe */
			std::string valStr(_rqstMsgBuffer[MsgIndex]->value->data(),
					_rqstMsgBuffer[MsgIndex]->value->size());
			mRTree->Put(keyStr, valStr);
			break;
		}
		case RqstOperation::PUTDISK: {
			unsigned int cluster = 0;
			std::string valStr(_rqstMsgBuffer[MsgIndex]->value->data(),
					_rqstMsgBuffer[MsgIndex]->value->size());
			/** @TODO: ppelplin: get free cluster for data */
			/*		cluster = get_free_cluster(); */
			/** @TODO: ppelplin: send i/o */

//			mDisk->Store(valStr, cluster, [&] {
			    /** @TODO: ppelplin: Update metadata in cb:
			     * value points to the used cluster
			     * location points to disk */
			    /*mRTree->Update(keyStr, cluster, disk);*/
//			});


			break;
		}
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
