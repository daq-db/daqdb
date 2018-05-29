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

void MinidaqNode::SetTimeTest(int s)
{
	tTest_s = s;
}

void MinidaqNode::SetTimeRamp(int s)
{
	tRamp_s = s;
}

void MinidaqNode::SetTimeIter(int us)
{
	tIter_us = us;
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
	MinidaqSample s;
	uint64_t avg_r;

	// Ramp up
	timerTest.Restart_s(tRamp_s);
	while (!timerTest.IsExpired()) {
		s.nRequests++;
		Task(executorId, c, c_err);
	}

	// Record samples
	timerTest.Restart_s(tTest_s);
	while (!timerTest.IsExpired() && !stopped) {
		// Timer precision per iteration
		avg_r = (s.nRequests + 10) / 10;
		s.Reset();
		c = 0;
		c_err = 0;
		timerSample.Restart_us(tIter_us);
		do {
			event_id += nTh;
			try {
				Task(event_id, c, c_err);
				s.nRequests++;
			}
			catch (...) {
				s.nErrRequests++;
			}
		} while (!stopped && ((s.nRequests % avg_r) || !timerSample.IsExpired()));
		s.nCompletions = c.load();
		s.nErrCompletions = c_err.load();
		s.interval_ns = timerSample.GetElapsed_ns();
		stats.RecordSample(s);
	}

	stopped = true;

	// Wait for all completions
	do {
		c = 0;
		c_err = 0;
		std::this_thread::sleep_for(std::chrono::nanoseconds(10 * s.interval_ns));
	} while (c || c_err); 

	return stats;
}

void MinidaqNode::Run()
{
	int i;

	Setup();

	std::cout << "Starting threads..." << std::endl;
	for (i = 0; i < nTh; i++) {
		futureVec.emplace_back(std::async(std::launch::async,
										  &MinidaqNode::Execute, this, i));
	}
}

void MinidaqNode::Wait()
{
	int i;
	
	std::cout << "Waiting..." << std::endl;
	for (const auto& f : futureVec) {
		f.wait();
	}
	std::cout << "Done!" << std::endl;
}

void MinidaqNode::Show()
{
	MinidaqStats total;
	int i = 0;

	for (auto& f : futureVec) {
		std::cout << "########################################" << std::endl;
		std::cout << "   STATS[" << i++ << "]:" << std::endl;
		std::cout << "########################################" << std::endl;
		auto s = f.get();
		s.Dump();
		total.Combine(s);
		std::cout << std::endl;
	}

	std::cout << "########################################" << std::endl;
	std::cout << "   STATS[ALL]:" << std::endl;
	std::cout << "########################################" << std::endl;
	total.Dump();
	std::cout << std::endl;
}

}
