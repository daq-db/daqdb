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

RqstPooler::RqstPooler(std::shared_ptr<pmemkv::KVEngine> Store) :
		mIsRunning(0), mThread(nullptr), mPmemkv(Store) {
	/** @TODO jradtke: ring size should be configurable? */
	mSubmitRing = spdk_ring_create(SPDK_RING_TYPE_MP_SC, 4096 * 4,
	SPDK_ENV_SOCKET_ID_ANY);
	Start();
}

RqstPooler::~RqstPooler() {
	spdk_ring_free(mSubmitRing);
	mIsRunning = 0;
	if (mThread != nullptr)
		mThread->join();
}

void RqstPooler::Start() {
	mIsRunning = 1;
	mThread = new std::thread(&RqstPooler::ThreadMain, this);
}

void RqstPooler::ThreadMain() {
	while (mIsRunning) {
		DequeueMsg();
		ProcessMsg();

		/** @TODO jradtke: Initial implementation, will be removed */
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
}

bool RqstPooler::EnqueueMsg(RqstMsg *Message) {
	size_t count = spdk_ring_enqueue(mSubmitRing, (void **) &Message, 1);
	return (count == 1);

	/** @TODO jradtke: Initial implementation, error handling not implemented */
}

void RqstPooler::DequeueMsg() {
	mDequedCount = spdk_ring_dequeue(mSubmitRing, (void **) mRqstMsgBuffer, 1);
	assert(mDequedCount < DEQUEUE_RING_LIMIT);
}

void RqstPooler::ProcessMsg() {
	for (unsigned short MsgIndex = 0; MsgIndex < mDequedCount; MsgIndex++) {
		std::string keyStr(mRqstMsgBuffer[MsgIndex]->key->data(),
				mRqstMsgBuffer[MsgIndex]->key->size());

		switch (mRqstMsgBuffer[MsgIndex]->op) {
		case RqstOperation::PUT: {
			/** @TODO jradtke: Not thread safe */
			std::string valStr(mRqstMsgBuffer[MsgIndex]->value->data(),
					mRqstMsgBuffer[MsgIndex]->value->size());
			mPmemkv->Put(keyStr, valStr);
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

		(*mRqstMsgBuffer[0]->cb_fn)(nullptr, FogKV::Status(FogKV::Code::Ok),
				*mRqstMsgBuffer[0]->key, *mRqstMsgBuffer[0]->value);
	}
}

} /* namespace FogKV */
