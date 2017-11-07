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
#include "IoMeter.h"

#include "workers/AepWorker.h"
#include "workers/DiskWorker.h"
#include "SimFogKV.h"

#include "AuxNode.h"
#include "MainNode.h"

using namespace std;
using namespace Fabric;

namespace po = boost::program_options;
namespace as = boost::asio;

LoggerPtr benchDragon(Logger::getLogger("benchmark"));

/*!
 *
 */
void logCpuUsage(const boost::system::error_code&,
		boost::asio::deadline_timer* cpuLogTimer, Dragon::CpuMeter* cpuMeter) {
	cpuMeter->logCpuUsage();

	cpuLogTimer->expires_at(
			cpuLogTimer->expires_at() + boost::posix_time::seconds(5));
	cpuLogTimer->async_wait(
			boost::bind(logCpuUsage, boost::asio::placeholders::error,
					cpuLogTimer, cpuMeter));
}

int main(int argc, const char *argv[]) {
	unsigned short port;
	unsigned short remotePort;
	std::string addr;
	std::string remoteAddr;

	std::string diskPath;
	unsigned int diskBuffSize;

	bool isMainNode = true;
	size_t buffSize;

	log4cxx::ConsoleAppender *consoleAppender = new log4cxx::ConsoleAppender(
			log4cxx::LayoutPtr(new log4cxx::SimpleLayout()));
	log4cxx::BasicConfigurator::configure(
			log4cxx::AppenderPtr(consoleAppender));
	log4cxx::Logger::getRootLogger()->setLevel(log4cxx::Level::getOff());

#if (1) // Cmd line parsing region
	po::options_description argumentsDescription { "Options" };
	argumentsDescription.add_options()("help,h", "Print help messages")(
			"port,p", po::value<unsigned short>(&port)->default_value(1234),
			"Port number")("addr,a",
			po::value<std::string>(&addr)->default_value("127.0.0.1"),
			"Address")("rport,r",
			po::value<unsigned short>(&remotePort)->default_value(1234),
			"Port number")("node,n",
			po::value<std::string>(&remoteAddr)->default_value("127.0.0.1"),
			"Address")("buff-size",
			po::value<size_t>(&buffSize)->default_value(16 * 1024 * 1024),
			"Ring buffer size")("aux,x", "Auxiliary node")("main,m",
			"Main node")("log,l", "Enable logging")("csv,c",
			"Enable creation of CSV file")("disk,d",
			po::value<std::string>(&diskPath)->default_value("/mnt/test_file"),
			"Path to value's DB file")("buff,b",
			po::value<unsigned int>(&diskBuffSize)->default_value(4 * 1024),
			"value's DB element size");

	bool enableCSV = false;
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
		if (parsedArguments.count("csv")) {
			enableCSV = true;
		}

		if (parsedArguments.count("main"))
			isMainNode = true;

		if (parsedArguments.count("aux"))
			isMainNode = false;

		po::notify(parsedArguments);
	} catch (po::error &parserError) {
		cerr << "Invalid arguments: " << parserError.what() << endl << endl;
		cerr << argumentsDescription << endl;
		return -1;
	}
#endif

	LOG4CXX_INFO(benchDragon, "Address: " + addr);
	LOG4CXX_INFO(benchDragon, "Port   : " + std::to_string(port));
	LOG4CXX_INFO(benchDragon,
			"Node   : "
					+ (isMainNode ? std::string("Main") : std::string("Aux")));

	as::io_service io_service;
	as::signal_set signals(io_service, SIGINT, SIGTERM);
	signals.async_wait(bind(&boost::asio::io_service::stop, &io_service));

	Dragon::CpuMeter cpuMeter(enableCSV);

	boost::asio::deadline_timer cpuLogTimer(io_service,
			boost::posix_time::seconds(5));
	cpuLogTimer.async_wait(
			boost::bind(logCpuUsage, boost::asio::placeholders::error,
					&cpuLogTimer, &cpuMeter));

	unique_ptr<AuxNode> auxNode;
	unique_ptr<MainNode> mainNode;
#define HELLO

#ifdef HELLO
	boost::asio::deadline_timer io_timer(io_service, boost::posix_time::seconds(1));
	int count = 0;
	std::function<void (const boost::system::error_code&)> io_timer_handler = [&] (const boost::system::error_code&) -> void {
		std::string hello("MSG " + std::to_string(count++));
		auxNode->write((uint8_t *)hello.c_str(), hello.size());

		uint8_t buff[4096];
		auxNode->read(buff, hello.size());
		std::string h((const char *)buff, hello.size());

		LOG4CXX_INFO(benchDragon, "READ: " + h);
		io_timer.async_wait(io_timer_handler);
	};

	std::string data;
#endif
	if (isMainNode) {
		mainNode = std::unique_ptr<MainNode>(new MainNode(addr, std::to_string(port), buffSize));
#ifdef HELLO
		mainNode->onWrite([&] (std::shared_ptr<RingBuffer> buff) -> void {
			data = "";
			buff->read(buff->occupied(), [&] (const uint8_t *buff, size_t l) -> ssize_t {
				data += std::string((const char *)buff, l);
			});
			LOG4CXX_INFO(benchDragon, "onWrite: " + data);
		});

		mainNode->onRead([&] (std::shared_ptr<RingBuffer> buff, size_t len) -> void {
			buff->write((const uint8_t *)data.c_str(), data.size());
			LOG4CXX_INFO(benchDragon, "onRead: " + data);
		});

#endif
		mainNode->start();
	} else {
		auxNode = std::unique_ptr<AuxNode>(new AuxNode(addr, std::to_string(port),
				remoteAddr, std::to_string(remotePort), buffSize));
		auxNode->onReady([&] () -> void {
#ifdef HELLO
			io_timer.async_wait(io_timer_handler);
#endif
		});
		auxNode->start();
	}

	LOG4CXX_INFO(benchDragon, "Start benchmark process");

	Dragon::SimFogKV simFog(diskPath, diskBuffSize);
	vector<char> buffer;
	buffer.assign(diskBuffSize, 'a');

	for (;;) {
		io_service.poll();
		if (io_service.stopped()) {
			break;
		}

		/*! example use

		for (int index = 0; index < 1000; ++index) {
			auto actionStatus = simFog.Put(std::to_string(index), buffer);
		}
		for (int index = 0; index < 1000; ++index) {
			string valuestr;
			auto actionStatus = simFog.Get(std::to_string(index), buffer);
		}

		float readStat = 0;
		float writeStat = 0;
		tie(readStat, writeStat) = simFog.getIoStat();
		if (readStat > 0)
		LOG4CXX_INFO(benchDragon,
			boost::format("Read[%1% op/s], Write[%2% op/s]") % readStat % writeStat);

		*/

		sleep(1);
	}
	cout << endl;

	return 0;
}
