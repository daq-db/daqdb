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

class MinidaqStats {
public:
	MinidaqStats();
	MinidaqStats(std::vector<MinidaqStats> &rVector);
	~MinidaqStats();

	void RecordSample(uint64_t nRequests, uint64_t nCompletions,
					  uint64_t nErrRequests, uint64_t nErrCompletions,
					  double interval);

	uint64_t GetIterations();
	uint64_t GetRequests();
	uint64_t GetErrRequests();
	uint64_t GetCompletions();
	uint64_t GetErrCompletions();
	std::vector<double> GetRps(); 
	std::vector<double> GetCps(); 
	std::vector<double> GetRpsErr(); 
	std::vector<double> GetCpsErr(); 
	void Dump();
	void DumpCsv();

private:
	std::vector<double> _rpsVec;
	std::vector<double> _cpsVec;
	std::vector<double> _rpsErrVec;
	std::vector<double> _cpsErrVec;

	uint64_t _nIterations = 0;
	uint64_t _nRequests = 0;
	uint64_t _nErrRequests = 0;
	uint64_t _nCompletions = 0;
	uint64_t _nErrCompletions = 0;
};

}
