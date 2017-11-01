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

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/format.hpp>
#include <boost/program_options.hpp>

#include "debug.h"
#include "CpuMeter.h"

using namespace std;

namespace po = boost::program_options;
namespace as = boost::asio;

/*!
 *
 */
void logCpuUsage(const boost::system::error_code&,
		boost::asio::deadline_timer* cpuLogTimer, Dragon::CpuMeter* cpuMeter) {
	cpuMeter->logCpuUsage();

	cpuLogTimer->expires_at(cpuLogTimer->expires_at() + boost::posix_time::seconds(5));
	cpuLogTimer->async_wait(
			boost::bind(logCpuUsage, boost::asio::placeholders::error, cpuLogTimer, cpuMeter));
}

int main(int argc, const char *argv[]) {
	LoggerPtr benchDragon(Logger::getLogger("benchmark"));

	log4cxx::ConsoleAppender *consoleAppender = new log4cxx::ConsoleAppender(
			log4cxx::LayoutPtr(new log4cxx::SimpleLayout()));
	log4cxx::BasicConfigurator::configure(
			log4cxx::AppenderPtr(consoleAppender));
	log4cxx::Logger::getRootLogger()->setLevel(log4cxx::Level::getOff());

#if (1) // Cmd line parsing region
	po::options_description argumentsDescription { "Options" };
	argumentsDescription.add_options()("help,h", "Print help messages")("log,l",
			"Enable logging");

	po::variables_map parsedArguments;
	try {
		po::store(po::parse_command_line(argc, argv, argumentsDescription),
				parsedArguments);
		if (parsedArguments.count("help")) {
			std::cout << argumentsDescription << endl;
			return 0;
		}
		if (parsedArguments.count("log")) {
			log4cxx::Logger::getRootLogger()->setLevel(
					log4cxx::Level::getDebug());
		}
		po::notify(parsedArguments);
	} catch (po::error &parserError) {
		cerr << "Invalid arguments: " << parserError.what() << endl << endl;
		cerr << argumentsDescription << endl;
		return -1;
	}
#endif

	LOG4CXX_INFO(benchDragon, "Start benchmark process");

	as::io_service io_service;
	as::signal_set signals(io_service, SIGINT, SIGTERM);
	signals.async_wait(bind(&boost::asio::io_service::stop, &io_service));

	Dragon::CpuMeter cpuMeter;

	boost::asio::deadline_timer cpuLogTimer(io_service, boost::posix_time::seconds(5));
	cpuLogTimer.async_wait(
			boost::bind(logCpuUsage, boost::asio::placeholders::error, &cpuLogTimer, &cpuMeter));

	for (;;) {
		io_service.poll();
		if (io_service.stopped()) {
			break;
		}
		// sleep(1);
	}
	cout << endl;

	LOG4CXX_INFO(benchDragon, "Closing benchmark process");

	return 0;
}
