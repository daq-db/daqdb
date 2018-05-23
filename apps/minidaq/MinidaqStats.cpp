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

#include "MinidaqStats.h"

using namespace std;

namespace FogKV {

bool operator <(const timespec& lhs, const timespec& rhs)
{
    if (lhs.tv_sec == rhs.tv_sec)
        return lhs.tv_nsec < rhs.tv_nsec;
    else
        return lhs.tv_sec < rhs.tv_sec;
}

MinidaqStats::MinidaqStats()
{
}

MinidaqStats::MinidaqStats(std::vector<MinidaqStats> &rVector)
{
	tStop.tv_sec = 0;
	tStop.tv_nsec = 0;
	GetTime(tStart);

	for (auto& r : rVector) {
		nSamples += r.GetSamples();
		nRequests += r.GetRequests();
		nErrRequests += r.GetErrRequests();
		nCompletions += r.GetCompletions();
		nErrCompletions += r.GetErrCompletions();
		if (!(tStart < r.GetStartTime())) {
			tStart = r.GetStartTime();
		}
		if (tStop < r.GetStopTime()) {
			tStop = r.GetStopTime();
		}
	}

	GetTimeDiff(tStop, tStart, tDiff);
}

MinidaqStats::~MinidaqStats()
{
}

void MinidaqStats::GetTime(timespec &tCurr)
{
	clock_gettime(CLOCK_MONOTONIC, &tCurr);
}

void MinidaqStats::GetTimeDiff(const timespec &t1, const timespec &t2, timespec &d)
{
	if ((t1.tv_nsec - t2.tv_nsec) < 0) {
		d.tv_sec = t1.tv_sec - t2.tv_sec - 1;
		d.tv_nsec = t1.tv_nsec - t2.tv_nsec + 1000000000UL;
	} else {
		d.tv_sec = t1.tv_sec - t2.tv_sec;
		d.tv_nsec = t1.tv_nsec - t2.tv_nsec;
	}
}

void MinidaqStats::SetStartTime()
{
	GetTime(tStart);
}

timespec MinidaqStats::GetStartTime()
{
	return tStart;
}

timespec MinidaqStats::GetStopTime()
{
	return tStop;
}

void MinidaqStats::SetElapsed()
{
	GetTime(tStop);
	GetTimeDiff(tStop, tStart, tDiff);
}

timespec MinidaqStats::GetElapsed()
{
	return tDiff;
}

bool MinidaqStats::IsEnough(uint64_t desired_time_s)
{
	return (tDiff.tv_sec >= desired_time_s);
}

void MinidaqStats::RecordRequest(timespec &lat)
{
	nRequests++;

	/** @todo add latency histogram */
}

void MinidaqStats::RecordErrRequest(timespec &lat)
{
	nErrRequests++;

	/** @todo add latency histogram */
}

void MinidaqStats::SetSamples(uint64_t n)
{
	nSamples = n;
}

void MinidaqStats::SetCompletions(uint64_t n)
{
	nCompletions = n;
}

void MinidaqStats::SetErrCompletions(uint64_t n)
{
	nErrCompletions = n;
}

uint64_t MinidaqStats::GetSamples()
{
	return nSamples;
}

uint64_t MinidaqStats::GetRequests()
{
	return nRequests;
}

uint64_t MinidaqStats::GetErrRequests()
{
	return nErrRequests;
}

uint64_t MinidaqStats::GetCompletions()
{
	return nCompletions;
}

uint64_t MinidaqStats::GetErrCompletions()
{
	return nErrCompletions;
}

void MinidaqStats::Dump()
{
	double cps;
	double rps;
	uint64_t n;
	double t;

	t = tDiff.tv_sec + double(tDiff.tv_nsec) / 1000000000.0;
	cps = double(nCompletions) / t;
	rps = double(nRequests) / t;

	std::cout << "Start time [sec]: " << tStart.tv_sec << "." << setfill('0')
									  << setw(9) << tStart.tv_nsec << std::endl;
	std::cout << "Duration [sec]: " << tDiff.tv_sec << "." << setfill('0')
									<< setw(9) << tDiff.tv_nsec << std::endl;
	std::cout << "Total samples: " << nSamples << std::endl;
	std::cout << "Issued requests: " << nRequests << std::endl;
	std::cout << "Error requests: " << nErrRequests << std::endl;
	std::cout << "Completions: " << nCompletions << std::endl;
	std::cout << "Completion errors: " << nErrCompletions << std::endl;
	std::cout << "Requests per second [1/sec]: " << rps << std::endl;
	std::cout << "Completions per second [1/sec]: " << cps << std::endl;
}

void MinidaqStats::DumpCsv()
{
}

}
