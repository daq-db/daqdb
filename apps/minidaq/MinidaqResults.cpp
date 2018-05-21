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

#include <iostream>
#include <iomanip>

#include "MinidaqResults.h"

using namespace std;

namespace FogKV {

MinidaqResults::MinidaqResults()
{
}

MinidaqResults::~MinidaqResults()
{
}

void MinidaqResults::SetStartTime()
{
	clock_gettime(CLOCK_MONOTONIC, &tStart);
}

void MinidaqResults::SetElapsed()
{
	struct timespec tCurr;

	clock_gettime(CLOCK_MONOTONIC, &tCurr);

	if ((tCurr.tv_nsec - tStart.tv_nsec) < 0) {
		tDiff.tv_sec = tCurr.tv_sec - tStart.tv_sec - 1;
		tDiff.tv_nsec = tCurr.tv_nsec - tStart.tv_nsec + 1000000000UL;
	} else {
		tDiff.tv_sec = tCurr.tv_sec - tStart.tv_sec;
		tDiff.tv_nsec = tCurr.tv_nsec - tStart.tv_nsec;
	}
}

bool MinidaqResults::IsEnough(uint64_t desired_time_s)
{
	return (tDiff.tv_sec >= desired_time_s);
}

void MinidaqResults::Dump()
{
	std::cout << "Start time (sec): " << tStart.tv_sec << "." << setfill('0')
									  << setw(9) << tStart.tv_nsec << std::endl;
	std::cout << "Duration (sec): " << tDiff.tv_sec << "." << setfill('0')
									<< setw(9) << tDiff.tv_nsec << std::endl;
}

}
