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
#include <time.h>
#include <cstdint>

namespace FogKV {

class MinidaqResults {
public:
	MinidaqResults();
	MinidaqResults(std::vector<MinidaqResults> &rVector);
	~MinidaqResults();

	static void GetTime(timespec &tCurr);
	static void GetTimeDiff(const timespec &t1, const timespec &t2, timespec &d);

	void SetStartTime();
	void SetElapsed();
	timespec GetStartTime();
	timespec GetElapsed();
	timespec GetStopTime();
	uint64_t GetErrors();
	uint64_t GetSamples();
	bool IsEnough(uint64_t desired_time_s);
	void RecordLatency(timespec &t);
	void RecordSample();
	void RecordError();
	void Dump();
	void DumpCsv();

private:
	timespec tStart;
	timespec tStop;
	timespec tDiff;

	uint64_t nErrors = 0;
	uint64_t nSamples = 0;
};

}
