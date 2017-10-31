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

#include <fstream>
#include <iostream>
#include <cmath>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/format.hpp>

#include "CpuMeter.h"
#include "debug.h"

namespace Dragon {

CpuMeter::CpuMeter() {

	boost::posix_time::ptime time_t_epoch(boost::gregorian::date(1970,1,1));
	boost::posix_time::ptime now = boost::posix_time::second_clock::local_time();
	boost::posix_time::time_duration timeDiff = now - time_t_epoch;

	std::stringstream filenameStream;
	filenameStream << "results_" << timeDiff.total_milliseconds() << ".csv";
	_csvFileName = filenameStream.str();

	std::ofstream file;
	file.open(_csvFileName);

	LOG4CXX_INFO(log4cxx::Logger::getRootLogger(), boost::format("Create CSV file [%1%]") % _csvFileName);

	Csv::Writer writer(file, ';');
	Csv::Row header;

	header.push_back("timestamp");
	header.push_back("cpu_usage");
	writer.insert(header);
	file.close();
}

CpuMeter::~CpuMeter() {
}

void CpuMeter::start() {
	_timer.start();
}

void CpuMeter::stop() {
	_timer.stop();
}

std::tuple<unsigned short, cpu_times> CpuMeter::getCpuUsage() {
	cpu_times resultTimes = _timer.elapsed();
	double cpuUsage = 100;

	auto idleTime = resultTimes.wall - (resultTimes.user + resultTimes.system);
	if (idleTime > 0) {
		cpuUsage = (double(resultTimes.user) + resultTimes.system)
				/ resultTimes.wall;
		cpuUsage *= 100;
	}

	return std::make_tuple(static_cast<unsigned short>(std::round(cpuUsage)),
			resultTimes);
}

std::string CpuMeter::format() {
	return _timer.format();
}

void CpuMeter::logCpuUsage() {
	std::ofstream file;
	file.open(_csvFileName, std::ios::out | std::ios::app);
	Csv::Writer writer(file, ';');
	Csv::Row row;

	row.push_back("1");
	row.push_back("test");
	writer.insert(row);
	file.close();
}

} /* namespace Dragon */
