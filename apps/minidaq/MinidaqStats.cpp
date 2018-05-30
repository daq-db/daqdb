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
#include <system_error>

#include "MinidaqStats.h"

using namespace std;

namespace FogKV {

MinidaqStats::MinidaqStats()
{
	int err;

	err = hdr_init(1, 100000000ULL, 5, &_histogramRps); 	
	if (err) {
		throw std::system_error(err, std::system_category());
	}
	err = hdr_init(1, 100000000ULL, 5, &_histogramCps); 	
	if (err) {
		throw std::system_error(err, std::system_category());
	}
	err = hdr_init(1, 100000000ULL, 5, &_histogramRpsErr); 	
	if (err) {
		throw std::system_error(err, std::system_category());
	}
	err = hdr_init(1, 100000000ULL, 5, &_histogramCpsErr); 	
	if (err) {
		throw std::system_error(err, std::system_category());
	}
}

MinidaqStats::MinidaqStats(const std::vector<MinidaqStats> &rVector) : MinidaqStats()
{
	for (const auto& r : rVector) {
		Combine(r);
	}
	
}

MinidaqStats::~MinidaqStats()
{
	if (_histogramRps) {
		hdr_close(_histogramRps);
	}
	if (_histogramCps) {
		hdr_close(_histogramCps);
	}
	if (_histogramRpsErr) {
		hdr_close(_histogramRpsErr);
	}
	if (_histogramCpsErr) {
		hdr_close(_histogramCpsErr);
	}
}

MinidaqStats::MinidaqStats(const MinidaqStats& other) :
	MinidaqStats()
{
	hdr_add(_histogramRps, other._histogramRps);
	hdr_add(_histogramCps, other._histogramCps);
	hdr_add(_histogramRpsErr, other._histogramRpsErr);
	hdr_add(_histogramCpsErr, other._histogramCpsErr);
	_nIterations = other._nIterations;
	_totalTime_ns = other._totalTime_ns;
}

MinidaqStats MinidaqStats::operator=(MinidaqStats&& other)
{
	_histogramRps = other._histogramRps;
	_histogramCps = other._histogramCps;
	_histogramRpsErr = other._histogramRpsErr;
	_histogramCpsErr = other._histogramCpsErr;
	_nIterations = other._nIterations;
	_totalTime_ns = other._totalTime_ns;

	other._histogramRps = nullptr;
	other._histogramCps = nullptr;
	other._histogramRpsErr = nullptr;
	other._histogramCpsErr = nullptr;
}

void MinidaqStats::Combine(const MinidaqStats& stats)
{
	if (!stats._nIterations) {
		return;
	}
	_nIterations += stats._nIterations;
	_totalTime_ns += stats._totalTime_ns;
	hdr_add(_histogramRps, stats._histogramRps);
	hdr_add(_histogramCps, stats._histogramCps);
	hdr_add(_histogramRpsErr, stats._histogramRpsErr);
	hdr_add(_histogramCpsErr, stats._histogramCpsErr);
}

void MinidaqStats::RecordSample(const MinidaqSample &s)
{
	bool ok;

	if (!s.interval_ns) {
		return;
	}

	if (s.nRequests) {
		ok = hdr_record_value(_histogramRps, (s.nRequests * 1000000000ULL) /
											  s.interval_ns); 
		if (!ok) {
			_nOverflows++;
		}
	}
	if (s.nCompletions) {
		ok = hdr_record_value(_histogramCps, (s.nCompletions * 1000000000ULL) /
											  s.interval_ns); 
		if (!ok) {
			_nOverflows++;
		}
	}
	if (s.nErrRequests) {
		ok = hdr_record_value(_histogramRpsErr, (s.nErrRequests * 1000000000ULL) /
												 s.interval_ns); 
		if (!ok) {
			_nOverflows++;
		}
	}
	if (s.nErrCompletions) {
		ok = hdr_record_value(_histogramCpsErr, (s.nErrCompletions * 1000000000ULL) /
												 s.interval_ns); 
		if (!ok) {
			_nOverflows++;
		}
	}
	_nIterations++;
	_totalTime_ns += s.interval_ns;

}

void MinidaqStats::Dump()
{
	double avg;

	std::cout << "Iterations: " << _nIterations << std::endl;
	std::cout << "Histogram overflows: " << _nOverflows << std::endl;

	if (!_nIterations) {
		return;
	}

	std::cout << "Average iteration time: "
			  << 1000.0 * double(_totalTime_ns) / double(_nIterations)
			  << " us" << std::endl;

	std::cout << "########################################################"
			  << std::endl;
	std::cout << "   Requests per Second [MOPS]" << std::endl;
	std::cout << "########################################################"
			  << std::endl;
	hdr_percentiles_print(_histogramRps, stdout, 1, 1000000.0, CLASSIC);
	std::cout << "########################################################"
			  << std::endl;
	std::cout << "   Completions per Second [MOPS]" << std::endl;
	std::cout << "########################################################"
			  << std::endl;
	hdr_percentiles_print(_histogramCps, stdout, 1, 1000000.0, CLASSIC);
	std::cout << "########################################################"
			  << std::endl;
	std::cout << "   Error Requests per Second [MOPS]" << std::endl;
	std::cout << "########################################################"
			  << std::endl;
	hdr_percentiles_print(_histogramRpsErr, stdout, 1, 1000000.0, CLASSIC);
	std::cout << "########################################################"
			  << std::endl;
	std::cout << "   Error Completions per Second [MOPS]" << std::endl;
	std::cout << "########################################################"
			  << std::endl;
	hdr_percentiles_print(_histogramCpsErr, stdout, 1, 1000000.0, CLASSIC);
}

void MinidaqStats::DumpCsv()
{
}

}
