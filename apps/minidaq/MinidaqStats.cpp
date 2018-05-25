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
#include <numeric>
#include <algorithm>

#include "MinidaqStats.h"

using namespace std;

namespace FogKV {

MinidaqStats::MinidaqStats()
{
}

MinidaqStats::MinidaqStats(std::vector<MinidaqStats> &rVector)
{
	std::vector<double> vec;

	for (auto& r : rVector) {
		_nIterations += r.GetIterations();
		_nRequests += r.GetRequests();
		_nErrRequests += r.GetErrRequests();
		_nCompletions += r.GetCompletions();
		_nErrCompletions += r.GetErrCompletions();
		vec = r.GetRps();
		_rpsVec.insert(_rpsVec.end(), vec.begin(), vec.end());
		vec = r.GetRpsErr();
		_rpsErrVec.insert(_rpsErrVec.end(), vec.begin(), vec.end());
		vec = r.GetCps();
		_cpsVec.insert(_cpsVec.end(), vec.begin(), vec.end());
		vec = r.GetCpsErr();
		_cpsErrVec.insert(_cpsErrVec.end(), vec.begin(), vec.end());
	}
}

MinidaqStats::~MinidaqStats()
{
}

uint64_t MinidaqStats::GetIterations()
{
	return _nIterations;
}

uint64_t MinidaqStats::GetRequests()
{
	return _nRequests;
}

uint64_t MinidaqStats::GetErrRequests()
{
	return _nErrRequests;
}

uint64_t MinidaqStats::GetCompletions()
{
	return _nCompletions;
}

uint64_t MinidaqStats::GetErrCompletions()
{
	return _nErrCompletions;
}

std::vector<double> MinidaqStats::GetRps()
{
	return _rpsVec;
}

std::vector<double> MinidaqStats::GetCps()
{
	return _cpsVec;
}

std::vector<double> MinidaqStats::GetRpsErr()
{
	return _rpsErrVec;
}

std::vector<double> MinidaqStats::GetCpsErr()
{
	return _cpsErrVec;
}

void MinidaqStats::RecordSample(uint64_t nRequests, uint64_t nCompletions,
								uint64_t nErrRequests, uint64_t nErrCompletions,
								double interval)
{
	if (interval <= 0.0)
		return;

	/** @todo replace with histogram */
	_nIterations++;
	_nRequests += nRequests;
	_nErrRequests += nErrRequests;
	_nCompletions += nCompletions;
	_nErrCompletions += nErrCompletions;
	_rpsVec.push_back(double(nRequests) / interval);
	_cpsVec.push_back(double(nCompletions) / interval);
	_rpsErrVec.push_back(double(nErrRequests) / interval);
	_cpsErrVec.push_back(double(nErrCompletions) / interval);
}

void MinidaqStats::Dump()
{
	double avg;

	std::cout << "Iterations: " << _nIterations << std::endl;
	std::cout << "Successful requests: " << _nRequests << std::endl;
	std::cout << "Successful completions: " << _nCompletions << std::endl;
	std::cout << "Error requests: " << _nErrRequests << std::endl;
	std::cout << "Error completions: " << _nErrCompletions << std::endl;

	avg = std::accumulate(_rpsVec.begin(), _rpsVec.end(), 0.0) / _rpsVec.size();
	std::cout << "Requests per second [MOPS] " << std::endl;
	std::cout << "  avg: " << avg << std::endl;
	std::cout << "  min: "
			  << *std::min_element(_rpsVec.begin(), _rpsVec.end()) << std::endl;
	std::cout << "  max: "
			  << *std::max_element(_rpsVec.begin(), _rpsVec.end()) << std::endl;

	avg = std::accumulate(_rpsErrVec.begin(), _rpsErrVec.end(), 0.0) / _rpsErrVec.size();
	std::cout << "Error requests per second [MOPS] " << std::endl;
	std::cout << "  avg: " << avg << std::endl;
	std::cout << "  min: "
			  << *std::min_element(_rpsErrVec.begin(), _rpsErrVec.end()) << std::endl;
	std::cout << "  max: "
			  << *std::max_element(_rpsErrVec.begin(), _rpsErrVec.end()) << std::endl;

	avg = std::accumulate(_cpsVec.begin(), _cpsVec.end(), 0.0) / _cpsVec.size();
	std::cout << "Completions per second [MOPS] " << std::endl;
	std::cout << "  avg: " << avg << std::endl;
	std::cout << "  min: "
			  << *std::min_element(_cpsVec.begin(), _cpsVec.end()) << std::endl;
	std::cout << "  max: "
			  << *std::max_element(_cpsVec.begin(), _cpsVec.end()) << std::endl;

	avg = std::accumulate(_cpsErrVec.begin(), _cpsErrVec.end(), 0.0) / _cpsErrVec.size();
	std::cout << "Error completions per second [MOPS] " << std::endl;
	std::cout << "  avg: " << avg << std::endl;
	std::cout << "  min: "
			  << *std::min_element(_cpsErrVec.begin(), _cpsErrVec.end()) << std::endl;
	std::cout << "  max: "
			  << *std::max_element(_cpsErrVec.begin(), _cpsErrVec.end()) << std::endl;
}

void MinidaqStats::DumpCsv()
{
}

}
