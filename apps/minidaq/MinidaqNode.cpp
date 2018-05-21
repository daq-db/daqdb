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

#include "MinidaqNode.h"

namespace FogKV {

MinidaqNode::MinidaqNode(KVStoreBase *kvs) :
	kvs(kvs)
{
}

MinidaqNode::~MinidaqNode()
{
}

void MinidaqNode::SetRampUp(int t)
{
	rampUpSeconds = t;
}

void MinidaqNode::SetDuration(int t)
{
	runSeconds = t;
}

void MinidaqNode::SetThreads(int n)
{
	nTh= n;
}

MinidaqResults MinidaqNode::Execute(int executorId)
{
	MinidaqResults r;
	timespec tStart;
	timespec tStop;
	timespec lat;

	// Ramp up
	r.SetStartTime();
	do {
		try {
			Task(executorId);
		}
		catch (...) {
		}
		r.SetElapsed();
	} while (!r.IsEnough(rampUpSeconds));

	// Record samples
	r.SetStartTime();
	do {
		r.GetTime(tStart);
		try {
			Task(executorId);
			r.RecordSample();
		}
		catch (...) {
			r.RecordError();
		}
		r.GetTime(tStop);
		r.GetTimeDiff(tStart, tStop, lat);
		r.RecordLatency(lat);
		r.SetElapsed();
	} while (!r.IsEnough(runSeconds) && !stopped);

	stopped = true;

	return r;
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
	std::vector<MinidaqResults> results;
	int i = 0;

	for (auto&& f : futureVec) {
		std::cout << "Results[" << i++ << "]:" << std::endl;
		results.push_back(f.get());
		results.back().Dump();
		std::cout << std::endl;
	}

	std::cout << "Results[all]:" << std::endl;
	MinidaqResults total(results);
	total.Dump();
	std::cout << std::endl;
}

}
