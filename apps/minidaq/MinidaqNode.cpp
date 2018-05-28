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

#include <future>
#include <thread>
#include <vector>
#include <iostream>
#include <atomic>

#include "MinidaqNode.h"
#include "MinidaqTimerHR.h"

#include <iomanip>

namespace FogKV {

MinidaqNode::MinidaqNode(KVStoreBase *kvs) :
	kvs(kvs), stopped(false)
{
}

MinidaqNode::~MinidaqNode()
{
}

void MinidaqNode::SetTimeTest(int tS)
{
	tTestS = tS;
}

void MinidaqNode::SetTimeRamp(int tS)
{
	tRampS = tS;
}

void MinidaqNode::SetTimeIter(int tMS)
{
	tIterMS = tMS;
}

void MinidaqNode::SetThreads(int n)
{
	nTh= n;
}

MinidaqStats MinidaqNode::Execute(int executorId)
{
	std::atomic<std::uint64_t> c_err;
	uint64_t event_id = executorId;
	std::atomic<std::uint64_t> c;
	MinidaqTimerHR timerSample;
	MinidaqTimerHR timerTest;
	MinidaqStats stats;
	uint64_t avg_r;
	uint64_t r_err;
	uint64_t r;
	double t;

	// Ramp up
	timerTest.RestartS(tRampS);
	while (!timerTest.IsExpired()) {
		r++;
		Task(executorId, c, c_err);
	}

	// Record samples
	timerTest.RestartS(tTestS);
	while (!timerTest.IsExpired() && !stopped) {
		// Timer precision per iteration
		avg_r = (r + 10) / 10;
		r = 0;
		c = 0;
		r_err = 0;
		c_err = 0;
		timerSample.RestartMS(tIterMS);
		do {
			event_id += nTh;
			try {
				Task(event_id, c, c_err);
				r++;
			}
			catch (...) {
				r_err++;
			}
		} while (!stopped && ((r % avg_r) || !timerSample.IsExpired()));
		t = timerSample.GetElapsedUS();
		stats.RecordSample(r, c, r_err, c_err, t);
	}

	stopped = true;

	// Wait for all completions
	while (c || c_err) {
		c = 0;
		c_err = 0;
		std::this_thread::sleep_for(std::chrono::milliseconds(100 * tIterMS));
	}

	return stats;
}

void MinidaqNode::Run()
{
	int i;

	Setup();

	for (i = 0; i < nTh; i++) {
		futureVec.emplace_back(std::async(std::launch::async,
										  &MinidaqNode::Execute, this, i));
	}
}

void MinidaqNode::Wait()
{
	int i;
	
	for (auto&& f : futureVec) {
		f.wait();
	}
}

void MinidaqNode::Show()
{
	std::vector<MinidaqStats> stats;
	int i = 0;

	for (auto&& f : futureVec) {
		std::cout << "stats[" << i++ << "]:" << std::endl;
		stats.push_back(f.get());
		stats.back().Dump();
		std::cout << std::endl;
	}

	std::cout << "stats[all]:" << std::endl;
	MinidaqStats total(stats);
	total.Dump();
	std::cout << std::endl;
}

}
