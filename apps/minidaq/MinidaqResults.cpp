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

bool operator <(const timespec& lhs, const timespec& rhs)
{
    if (lhs.tv_sec == rhs.tv_sec)
        return lhs.tv_nsec < rhs.tv_nsec;
    else
        return lhs.tv_sec < rhs.tv_sec;
}

MinidaqResults::MinidaqResults()
{
}

MinidaqResults::MinidaqResults(std::vector<MinidaqResults> &rVector)
{
	nSamples = 0;
	tStop.tv_sec = 0;
	tStop.tv_nsec = 0;
	GetTime(tStart);

	for (auto& r : rVector) {
		nSamples += r.GetSamples();
		nErrors += r.GetErrors();
		if (!(tStart < r.GetStartTime())) {
			tStart = r.GetStartTime();
		}
		if (tStop < r.GetStopTime()) {
			tStop = r.GetStopTime();
		}
	}

	GetTimeDiff(tStop, tStart, tDiff);
}

MinidaqResults::~MinidaqResults()
{
}

void MinidaqResults::GetTime(timespec &tCurr)
{
	clock_gettime(CLOCK_MONOTONIC, &tCurr);
}

void MinidaqResults::GetTimeDiff(const timespec &t1, const timespec &t2, timespec &d)
{
	if ((t1.tv_nsec - t2.tv_nsec) < 0) {
		d.tv_sec = t1.tv_sec - t2.tv_sec - 1;
		d.tv_nsec = t1.tv_nsec - t2.tv_nsec + 1000000000UL;
	} else {
		d.tv_sec = t1.tv_sec - t2.tv_sec;
		d.tv_nsec = t1.tv_nsec - t2.tv_nsec;
	}
}

void MinidaqResults::SetStartTime()
{
	GetTime(tStart);
}

timespec MinidaqResults::GetStartTime()
{
	return tStart;
}

timespec MinidaqResults::GetStopTime()
{
	return tStop;
}

void MinidaqResults::SetElapsed()
{
	GetTime(tStop);
	GetTimeDiff(tStop, tStart, tDiff);
}

timespec MinidaqResults::GetElapsed()
{
	return tDiff;
}

bool MinidaqResults::IsEnough(uint64_t desired_time_s)
{
	return (tDiff.tv_sec >= desired_time_s);
}

void MinidaqResults::RecordLatency(timespec &t)
{
}

void MinidaqResults::RecordSample()
{
	nSamples++;
}

uint64_t MinidaqResults::GetSamples()
{
	return nSamples;
}

void MinidaqResults::RecordError()
{
	nErrors++;
}

uint64_t MinidaqResults::GetErrors()
{
	return nErrors;
}

void MinidaqResults::Dump()
{
	double ops;
	double t;

	t = tDiff.tv_sec + double(tDiff.tv_nsec) / 1000000000.0;
	ops = (nSamples - nErrors) / t;

	std::cout << "Start time [sec]: " << tStart.tv_sec << "." << setfill('0')
									  << setw(9) << tStart.tv_nsec << std::endl;
	std::cout << "Duration [sec]: " << tDiff.tv_sec << "." << setfill('0')
									<< setw(9) << tDiff.tv_nsec << std::endl;
	std::cout << "Total samples: " << nSamples << std::endl;
	std::cout << "Successful samples: " << nSamples - nErrors << std::endl;
	std::cout << "Error samples: " << nErrors << std::endl;
	std::cout << "Operations per second [1/sec]: " << ops << std::endl;
}

void MinidaqResults::DumpCsv()
{
}

}
