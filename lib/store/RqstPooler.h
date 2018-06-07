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

#include <atomic>
#include <cstdint>
#include <thread>

#include "RTreeEngine.h"
#include "spdk/env.h"
#include "spdk/io_channel.h"
#include "spdk/queue.h"
#include "OffloadEngine.h"

#include <FogKV/KVStoreBase.h>

#define DEQUEUE_RING_LIMIT 1024

namespace FogKV {

enum class RqstOperation
	: std::int8_t {NONE = 0, GET = 1, PUT = 2, UPDATE = 3, DELETE = 4
};

class RqstPoolerInterface {
    virtual void StartThread() = 0;

    virtual bool EnqueueMsg(RqstMsg *Message) = 0;
    virtual void DequeueMsg() = 0;
    virtual void ProcessMsg() = 0;
};

class RqstPooler {
public:
	RqstPooler(std::shared_ptr<FogKV::RTreeEngine> Store);
	virtual ~RqstPooler();

	void Start();
	bool EnqueueMsg(RqstMsg *Message);

	std::atomic<int> isRunning;

	/** @TODO jradtke: Do we need completion queue too? */
	struct spdk_ring *submitRing;

private:
	void ThreadMain(void);
	void DequeueMsg();
	void ProcessMsg();

	std::thread *_thread;
	std::shared_ptr<FogKV::RTreeEngine> mRTree;
	std::shared_ptr<FogKV::OffloadEngine> mDisk;

    std::thread *_thread;
    size_t _cpuCore = 0;
};

} /* namespace FogKV */
