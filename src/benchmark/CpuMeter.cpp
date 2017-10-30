/**
 * Copyright 2017, Intel Corporation
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
#include "CpuMeter.h"
#include <boost/regex.hpp>

namespace Dragon
{

CpuMeter::CpuMeter()
{
}

CpuMeter::~CpuMeter()
{
}

void
CpuMeter::start()
{
	_timer.start();
}

void
CpuMeter::stop()
{
	_timer.stop();
}

std::tuple<float, cpu_times>
CpuMeter::getCpuUsage()
{
	cpu_times resultTimes = _timer.elapsed();
	auto resultUsage = float(
		(resultTimes.wall - (resultTimes.user + resultTimes.system)) /
		(100.f * resultTimes.wall));

	return std::make_tuple(resultUsage, resultTimes);
}

std::vector<long long>
CpuMeter::getGlobalCpuUsage()
{
	// std::ifstream in("/proc/stat");
	std::vector<long long> result;

	// This might broke if there are not 8 columns in /proc/stat
	// user: normal processes executing in user mode
	// nice: niced processes executing in user mode
	// system: processes executing in kernel mode
	// idle: twiddling thumbs
	// iowait: waiting for I/O to complete
	// irq: servicing interrupts
	// softirq: servicing softirqs

	//	boost::regex reg("cpu(\\d+) (\\d+) (\\d+) (\\d+) (\\d+) (\\d+) (\\d+) "
	//		 "(\\d+) (\\d+)");
	//
	//	std::string line;
	//	while (std::getline(in, line)) {
	//
	//		boost::smatch match;
	//		if (boost::regex_match(line, match, reg)) {
	//
	//			long long all_time = 0;
	//			for (unsigned )
	//			long long idle_time =
	//				boost::lexical_cast<long long>(match[5]);
	//
	//			result.push_back(100 - idle_time);
	//		}
	//	}

	return result;
}

std::string
CpuMeter::format()
{
	return _timer.format();
}

} /* namespace Dragon */
