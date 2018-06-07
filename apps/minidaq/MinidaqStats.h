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
#include "hdr_histogram.h"

namespace FogKV {

enum MinidaqMetricType {
	MINIDAQ_METRIC_RPS,
	MINIDAQ_METRIC_RPS_ERR,
	MINIDAQ_METRIC_CPS,
	MINIDAQ_METRIC_CPS_ERR,
};

struct MinidaqSample {
	MinidaqSample() = default;
	MinidaqSample(uint64_t r, uint64_t c, uint64_t e_r, uint64_t e_c, uint64_t ns) :
		nRequests(r), nCompletions(c), nErrRequests(e_r),
		nErrCompletions(e_c), interval_ns(ns) {};
	void Reset() {
		nRequests = 0;
		nCompletions = 0;
		nErrRequests = 0;
		nErrCompletions = 0;
		interval_ns = 0;
	}
	uint64_t nRequests = 0;
	uint64_t nCompletions = 0;
	uint64_t nErrRequests = 0;
	uint64_t nErrCompletions = 0;
	uint64_t interval_ns = 0;
};

class MinidaqStats {
public:
	MinidaqStats();
	MinidaqStats(const std::vector<MinidaqStats> &rVector);
	~MinidaqStats();
	MinidaqStats (const MinidaqStats& other);
	MinidaqStats operator=(MinidaqStats&& other);

	void RecordSample(const MinidaqSample &s);
	void Combine(const MinidaqStats& stats);

	void Dump();
	void Dump(const std::string& fp);
	void DumpSummary();
	void DumpSummary(const std::string& fp, const std::string& name);
	void DumpSummaryHeader();

private:
	void _DumpCsv(const std::string &fp, enum MinidaqMetricType histo_type);
	void _DumpSummary(enum MinidaqMetricType histo_type);
	void _DumpSummaryCsv(std::fstream &f, enum MinidaqMetricType histo_type);

	struct hdr_histogram* _histogramRps = nullptr;
	struct hdr_histogram* _histogramCps = nullptr;
	struct hdr_histogram* _histogramRpsErr = nullptr;
	struct hdr_histogram* _histogramCpsErr = nullptr;

	uint64_t _nIterations = 0;
	uint64_t _totalTime_ns = 0;
	uint64_t _nOverflows = 0;
};

}
