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

#include <vector>
#include <future>
#include <atomic>
#include <string>

#include "MinidaqStats.h"
#include "FogKV/KVStoreBase.h"

namespace FogKV {

struct MinidaqKey {
	MinidaqKey() = default;
	MinidaqKey(uint64_t e, uint16_t s, uint16_t r) :
		event_id(e), subdetector_id(s), run_id(r) {}
	uint64_t event_id;
	uint16_t subdetector_id;
	uint16_t run_id;
};

class MinidaqNode {
public:
	MinidaqNode(KVStoreBase *kvs);
	virtual ~MinidaqNode();

	void Run();
	void Wait();
	void Show();
	void SetTimeTest(int s);
	void SetTimeIter(int us);
	void SetTimeRamp(int s);
	void SetThreads(int n);

protected:
	virtual void Task(uint64_t eventId, std::atomic<std::uint64_t> &cnt,
					  std::atomic<std::uint64_t> &cntErr) = 0;
	virtual void Setup() = 0;
	virtual std::string GetType() = 0;

	KVStoreBase *kvs;
	int nTh = 0; // number of worker threads
	int id = 1; // global ID of the node
	int nReadoutNodes = 1; // global number of Readout Nodes;
	int runId = 599;

private:
	MinidaqStats Execute(int nThreads);

	int tTest_s = 0; // desired test duration in seconds
	int tRamp_s = 0; // desired test ramp duration in seconds
	int tIter_us = 0; // desired iteration time in microseconds
	std::atomic_bool stopped; // signals first thread stopped execution

	std::vector<std::future<MinidaqStats>> futureVec;
};

}
