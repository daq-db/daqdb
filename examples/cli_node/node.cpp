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
#include <boost/program_options.hpp>
#include <boost/format.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/algorithm/string.hpp>

#include <db/FogSrv.h>

#include "debug.h"
#include "nodeCli.h"

using namespace std;
using boost::format;
using namespace boost::algorithm;

namespace po = boost::program_options;

LoggerPtr loggerNode(Logger::getLogger( "dragon"));

namespace
{
const unsigned short dhtBackBonePort = 11000;
}

int
main(int argc, const char *argv[])
{
	unsigned short inputPort;
	unsigned short nodeId = 0;
	auto dhtPort = dhtBackBonePort;
	bool interactiveMode = false;

	log4cxx::ConsoleAppender *consoleAppender =
		new log4cxx::ConsoleAppender(
			log4cxx::LayoutPtr(new log4cxx::SimpleLayout()));
	log4cxx::BasicConfigurator::configure(
		log4cxx::AppenderPtr(consoleAppender));
	log4cxx::Logger::getRootLogger()->setLevel(log4cxx::Level::getOff());

#if (1) // Cmd line parsing region
	po::options_description argumentsDescription{"Options"};
	argumentsDescription.add_options()
			("help,h", "Print help messages")
			("port,p", po::value<unsigned short>(&inputPort), "Node Communication port")
			("dht,d", po::value<unsigned short>(&dhtPort), "DHT Communication port")
			("nodeid,n", po::value<unsigned short>(&nodeId)->default_value(0) , "Node ID used to match database file. If not set DB file will be removed when node stopped.")
			("interactive,i", "Enable interactive mode")
			("log,l", "Enable logging");

	po::variables_map parsedArguments;
	try {
		po::store(po::parse_command_line(argc, argv,
						 argumentsDescription),
			  parsedArguments);

		if (parsedArguments.count("help")) {
			std::cout << argumentsDescription << endl;
			return 0;
		}
		if (parsedArguments.count("interactive")) {
			interactiveMode = true;
		}
		if (parsedArguments.count("log")) {
			log4cxx::Logger::getRootLogger()->setLevel(log4cxx::Level::getDebug());
		}

		po::notify(parsedArguments);
	} catch (po::error &parserError) {
		cerr << "Invalid arguments: " << parserError.what() << endl
		     << endl;
		cerr << argumentsDescription << endl;
		return -1;
	}
#endif

	as::io_service io_service;
	as::signal_set signals(io_service, SIGINT, SIGTERM);
	signals.async_wait(	boost::bind(&boost::asio::io_service::stop, &io_service));
	shared_ptr<FogKV::FogSrv> spDragonSrv(
		new FogKV::FogSrv(io_service, nodeId));

	LOG4CXX_INFO(loggerNode,
		     format("DHT node (id=%1%) is running on %2%:%3%") %
			     spDragonSrv->getDhtId() % spDragonSrv->getIp() %
			     spDragonSrv->getPort());
	LOG4CXX_INFO(loggerNode, format("Waiting for requests on port %1%.") %
			     spDragonSrv->getDragonPort());

	if (!interactiveMode) {
		spDragonSrv->run();
	} else {
#if (1) // interactive mode
		FogKV::nodeCli dragonCli(spDragonSrv);
		while (dragonCli()) {
			if (spDragonSrv->stopped()) {
				break;
			}
		}

#endif
	}

	LOG4CXX_INFO(loggerNode,
		     format("Closing DHT node (id=%1%) on %2%:%3%") %
			     spDragonSrv->getDhtId() % spDragonSrv->getIp() %
			     spDragonSrv->getPort());

	return 0;
}
