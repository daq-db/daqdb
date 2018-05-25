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
	std::atomic<std::uint64_t> c;
	MinidaqTimerHR timerSample;
	MinidaqTimerHR timerTest;
	MinidaqStats stats;
	bool err = false;
	double avg_cps;
	double avg_rps;
	double avg_cps_err;
	double avg_rps_err;
	uint64_t r_err;
	uint64_t s = 0;
	uint64_t n;
	uint64_t r;
	int i = 0;
	double t;

	// Ramp up
	timerTest.RestartS(tRampS);
	while (!timerTest.IsExpired()) {
		Task(executorId, c, c_err);
	}

	// Record samples
	s = 0;
	timerTest.RestartS(tTestS);
	while (!timerTest.IsExpired() && !stopped) {
		s++;
		r = 0;
		c = 0;
		r_err = 0;
		c_err = 0;
		timerSample.RestartMS(tIterMS);
		while (!timerSample.IsExpired() && !stopped) {
			n++;
			try {
				Task(executorId, c, c_err);
				r++;
			}
			catch (...) {
				r_err++;
			}
		}
		t = timerSample.GetElapsedUS();
		stats.RecordSample(r, c, r_err, c_err, t);
	}

	stopped = true;

	/*
	r.SetSamples(n);
	r.SetCompletions(c);
	r.SetErrCompletions(c_err);
	*/
	// wait without recording for all completions

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
