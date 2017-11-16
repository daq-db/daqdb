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

CpuMeter::CpuMeter(bool enableCSV) :
		_csvEnabled(enableCSV) {

	if (enableCSV) {
		boost::posix_time::ptime time_t_epoch(
				boost::gregorian::date(1970, 1, 1));
		boost::posix_time::ptime now =
				boost::posix_time::second_clock::local_time();
		boost::posix_time::time_duration timeDiff = now - time_t_epoch;

		std::stringstream filenameStream;
		filenameStream << "results_" << timeDiff.total_milliseconds() << ".csv";
		_csvFileName = filenameStream.str();

		std::ofstream file;
		file.open(_csvFileName);

		LOG4CXX_INFO(log4cxx::Logger::getRootLogger(),
				boost::format("Create CSV file [%1%]") % _csvFileName);

		Csv::Writer writer(file, ';');
		Csv::Row header;

		header.push_back("timestamp");
		header.push_back("cpu_usage");
		header.push_back("cpu_active_time");
		header.push_back("cpu_idle_time");
		header.push_back("cpu_totat_time");
		header.push_back("proc_cpu_usage");
		header.push_back("proc_cpu_time_system");
		header.push_back("proc_cpu_time_user");
		header.push_back("proc_cpu_time_wall");

		header.push_back("io_put_items_per_sec");
		header.push_back("io_get_items_per_sec");

		writer.insert(header);
		file.close();
	} else {
		LOG4CXX_INFO(log4cxx::Logger::getRootLogger(), "CSV disabled");
	}

	_spLastSnapshot.reset(new CPUSnapshot());
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

void CpuMeter::logCpuUsage(Dragon::SimFogKV* simFog) {

	boost::posix_time::ptime now =
			boost::posix_time::second_clock::local_time();

	auto timestamp = boost::posix_time::to_iso_extended_string(now);

	CPUSnapshot* newCpuSnapshot = new CPUSnapshot();
	auto activeTime = static_cast<unsigned int>(std::rint(
			newCpuSnapshot->GetActiveTimeTotal()
					- _spLastSnapshot->GetActiveTimeTotal()));
	auto idleTime = static_cast<unsigned int>(std::rint(
			newCpuSnapshot->GetIdleTimeTotal()
					- _spLastSnapshot->GetIdleTimeTotal()));
	auto totalTime = static_cast<unsigned int>(std::rint(
			newCpuSnapshot->GetTotalTimeTotal()
					- _spLastSnapshot->GetTotalTimeTotal()));
	auto totalCpuUsage = 100.f * activeTime / totalTime;


	float readStat = 0;
	float writeStat = 0;
	std::tie(readStat, writeStat) = simFog->getIoStat();

	unsigned short cpuUsage = 0;
	cpu_times cpuTimes;
	std::tie(cpuUsage, cpuTimes) = getCpuUsage();

	if (_csvEnabled) {
		std::ofstream file;
		file.open(_csvFileName, std::ios::out | std::ios::app);
		Csv::Writer writer(file, ';');
		Csv::Row row;
		row.push_back(timestamp);

		row.push_back(std::to_string(totalCpuUsage));
		row.push_back(std::to_string(activeTime));
		row.push_back(std::to_string(idleTime));
		row.push_back(std::to_string(totalTime));

		row.push_back(std::to_string(cpuUsage));
		row.push_back(std::to_string(cpuTimes.system));
		row.push_back(std::to_string(cpuTimes.user));
		row.push_back(std::to_string(cpuTimes.wall));

		row.push_back(std::to_string(readStat));
		row.push_back(std::to_string(writeStat));

		writer.insert(row);
		file.close();
	}

	LOG4CXX_DEBUG(log4cxx::Logger::getRootLogger(),
			boost::format(
					"%5%: Process CPU usage = %1%%%, system=%3%, user=%4%, wall=%2%")
					% cpuUsage % cpuTimes.wall % cpuTimes.system % cpuTimes.user
					% timestamp);
	LOG4CXX_DEBUG(log4cxx::Logger::getRootLogger(),
			boost::format(
					"%5%: System CPU usage = %1%%%, active=%2%, idle=%3%, wall=%4%")
					% totalCpuUsage % activeTime % idleTime % totalTime
					% timestamp);

	_timer.start();
	_spLastSnapshot.release();
	_spLastSnapshot.reset(newCpuSnapshot);
}

} /* namespace Dragon */
