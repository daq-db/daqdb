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

#include <asio/ip/tcp.hpp>
#include <asio/signal_set.hpp>

#include <boost/program_options.hpp>
#include <chrono>

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

#ifdef FOGKV_USE_LOG4CXX
LoggerPtr benchDragon(Logger::getLogger("benchmark"));
#endif

/*!
 *
 */
void logCpuUsage(asio::deadline_timer* cpuLogTimer, DaqDB::CpuMeter* cpuMeter, DaqDB::SimFogKV* simFog, Node* node) {

	double txU = node->getAvgTxBuffUsage(true) * 100.0;
	double rxU = node->getAvgRxBuffUsage(true) * 100.0;
	double txMsgU = node->getAvgTxMsgUsage(true) * 100.0;
#ifdef FOGKV_USE_LOG4CXX
	LOG4CXX_INFO(benchDragon, "TX: " + std::to_string(txU) + " RX: " + std::to_string(rxU) + " TX MSG: " + std::to_string(txMsgU));
#endif

	cpuLogTimer->expires_at(
			cpuLogTimer->expires_at() + boost::posix_time::seconds(1));
	cpuLogTimer->async_wait(
			std::bind(logCpuUsage, cpuLogTimer, cpuMeter, simFog, node));
}

int main(int argc, const char *argv[]) {
	unsigned short port;
	unsigned short remotePort;
	std::string addr;
	std::string remoteAddr;

	std::string diskPath;
	unsigned int diskBuffSize;

	bool isMainNode = true;
	bool noStorage = false;
	bool cpuUsage = false;
	bool logMsg = false;
	size_t txBuffCount;
	size_t rxBuffCount;
	unsigned int put_iter;
	unsigned int get_iter;
	unsigned int iter;
	size_t buffSize;

#ifdef FOGKV_USE_LOG4CXX
	log4cxx::ConsoleAppender *consoleAppender = new log4cxx::ConsoleAppender(
			log4cxx::LayoutPtr(new log4cxx::SimpleLayout()));
	log4cxx::BasicConfigurator::configure(
			log4cxx::AppenderPtr(consoleAppender));
	log4cxx::Logger::getRootLogger()->setLevel(log4cxx::Level::getOff());
#endif

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
			"value's DB element size")
			("no-storage", "Do not use storage")
			("cpu-usage", "Show cpu usage")
			("put-iter", po::value<unsigned int>(&put_iter)->default_value(10000), "Number of PUT iterations")
			("get-iter", po::value<unsigned int>(&get_iter)->default_value(10000), "Number of GET iterations")
			("iter", po::value<unsigned int>(&iter)->default_value(10000), "Number of iterations")
			("log-msg", "Log messages")
			("rx-buff-count", po::value<size_t>(&rxBuffCount)->default_value(16), "Number of Rx buffers")
			("tx-buff-count", po::value<size_t>(&txBuffCount)->default_value(16), "Number of Tx buffers");

	bool enableCSV = false;
	po::variables_map parsedArguments;
	try {
		po::store(po::parse_command_line(argc, argv, argumentsDescription),
				parsedArguments);
		if (parsedArguments.count("help")) {
			std::cout << argumentsDescription << endl;
			return 0;
		}

#ifdef FOGKV_USE_LOG4CXX
		if (parsedArguments.count("log")) {
			log4cxx::Logger::getRootLogger()->setLevel(
					log4cxx::Level::getDebug());
		}
#endif
		if (parsedArguments.count("csv")) {
			enableCSV = true;
		}

		if (parsedArguments.count("main"))
			isMainNode = true;

		if (parsedArguments.count("aux"))
			isMainNode = false;

		if (parsedArguments.count("no-storage"))
			noStorage = true;

		if (parsedArguments.count("cpu-usage"))
			cpuUsage = true;

		if (parsedArguments.count("log-msg"))
			logMsg = true;

		po::notify(parsedArguments);
	} catch (po::error &parserError) {
		cerr << "Invalid arguments: " << parserError.what() << endl << endl;
		cerr << argumentsDescription << endl;
		return -1;
	}
#endif

#ifdef FOGKV_USE_LOG4CXX
	LOG4CXX_INFO(benchDragon, "Address: " + addr);
	LOG4CXX_INFO(benchDragon, "Port   : " + std::to_string(port));
	LOG4CXX_INFO(benchDragon,
			"Node   : "
					+ (isMainNode ? std::string("Main") : std::string("Aux")));
#endif

	asio::io_service io_service;
	asio::signal_set signals(io_service, SIGINT, SIGTERM);
	signals.async_wait(bind(&asio::io_service::stop, &io_service));

	DaqDB::CpuMeter cpuMeter(enableCSV);
	DaqDB::SimFogKV simFog(diskPath, diskBuffSize);

	asio::deadline_timer cpuLogTimer(io_service,
			boost::posix_time::seconds(1));

	vector<char> buffer;
	buffer.assign(diskBuffSize, 'a');

	unique_ptr<AuxNode> auxNode;
	unique_ptr<MainNode> mainNode;

	int count = 0;
	if (isMainNode) {
		mainNode = std::unique_ptr<MainNode>(new MainNode(addr, std::to_string(port), buffSize,
				txBuffCount, rxBuffCount, logMsg));

		mainNode->onPut([&] (const std::string &key, const std::vector<char> &value) -> void {
			if (noStorage)
				return;
			auto actionStatus = simFog.Put(key, value);
		});

		mainNode->onGet ([&] (const std::string &key, std::vector<char> &value) -> void {
			if (noStorage) {
				value.resize(diskBuffSize);
				return;
			}
			auto actionStatus = simFog.Get(key, value);
		});

		mainNode->start();
	} else {
		auxNode = std::unique_ptr<AuxNode>(new AuxNode(addr, std::to_string(port),
				remoteAddr, std::to_string(remotePort),
				buffSize, txBuffCount, rxBuffCount, logMsg));

		auxNode->start();
	}

	if (cpuUsage) {
		cpuLogTimer.async_wait(
				std::bind(logCpuUsage, &cpuLogTimer, &cpuMeter, &simFog, mainNode.get()));
	}

#ifdef FOGKV_USE_LOG4CXX
	LOG4CXX_INFO(benchDragon, "Start benchmark process");
#endif

	for (;;) {
		io_service.poll();
		if (io_service.stopped()) {
			break;
		}

		if (iter == 0)
			break;
		else
			iter--;

		if (!isMainNode) {
			if (put_iter)
			{
				std::chrono::high_resolution_clock::time_point t1 =
					std::chrono::high_resolution_clock::now();

				for (int index = 0; index < put_iter; ++index) {
					auxNode->put(std::to_string(index), buffer);
				}

				std::chrono::high_resolution_clock::time_point t2 =
					std::chrono::high_resolution_clock::now();

				std::chrono::duration<double> time = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1);
				double lat = time.count() / put_iter;
				double iops = 1.0 / lat;
				double txU = auxNode->getAvgTxBuffUsage(true) * 100.0;
				double rxU = auxNode->getAvgRxBuffUsage(true) * 100.0;
				double txMsgU = auxNode->getAvgTxMsgUsage(true) * 100.0;

#ifdef FOGKV_USE_LOG4CXX
				LOG4CXX_INFO(benchDragon, "PUT LAT: " +  std::to_string(lat*1000000.0) + " us IOPS: " + std::to_string(iops) + " 1/s " +
					"TX: " + std::to_string(txU) + "% RX: " + std::to_string(rxU) +
				       " TX MSG: " + std::to_string(txMsgU)	
				);
#endif
			}

			if (get_iter)
			{
				std::chrono::high_resolution_clock::time_point t1 =
					std::chrono::high_resolution_clock::now();

				for (int index = 0; index < get_iter; ++index) {
					auxNode->get(std::to_string(index), buffer);
				}

				std::chrono::high_resolution_clock::time_point t2 =
					std::chrono::high_resolution_clock::now();

				std::chrono::duration<double> time = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1);
				double lat = time.count() / get_iter;
				double iops = 1.0 / lat;
				double txU = auxNode->getAvgTxBuffUsage(true) * 100.0;
				double rxU = auxNode->getAvgRxBuffUsage(true) * 100.0;

#ifdef FOGKV_USE_LOG4CXX
				LOG4CXX_INFO(benchDragon, "GET LAT: " +  std::to_string(lat*1000000.0) + " us IOPS: " + std::to_string(iops) + " 1/s " +
					"TX: " + std::to_string(txU) + "% RX: " + std::to_string(rxU) 
				);
#endif
			}

		} else {
			sleep(1);
		}
	}

	cout << endl;

	return 0;
}
