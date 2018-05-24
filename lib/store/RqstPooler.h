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

#pragma once

#include <thread>
#include <atomic>
#include <cstdint>

#include "spdk/env.h"
#include "spdk/io_channel.h"
#include "spdk/queue.h"

#include <FogKV/KVStoreBase.h>

namespace FogKV {

enum class RqstOperation
	: std::int8_t {NONE = 0, GET = 1, PUT = 2, UPDATE = 3, DELETE = 4
};

class RqstMsg {
public:
	RqstMsg(RqstOperation op, Key *key, Value *value,
			KVStoreBase::KVStoreBasePutCallback *cb_fn);
	Key *key = nullptr;
	Value *value = nullptr;
	KVStoreBase::KVStoreBasePutCallback *cb_fn = nullptr;
	RqstOperation op = RqstOperation::NONE;
};

class RqstPooler {
public:
	RqstPooler();
	virtual ~RqstPooler();
	void Start();
	bool EnqueueMsg(RqstMsg *Message);

	/** @TODO jradtke: Change to enum with proper states */
	std::atomic_int mState;
	/** @TODO jradtke: Do we need completion queue too? */
	struct spdk_ring *mSubmitRing;
	std::thread *mThread;
private:
	void ThreadMain(void);
	void DequeueMsg();

	/**
	 * @TODO jradtke: How many messages to store?
	 * Maybe circular buffer will be better choice here?
	 */
	RqstMsg *mRqstMsgBuffer[1024];
};

} /* namespace FogKV */
