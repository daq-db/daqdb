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
#include <boost/assign/list_of.hpp>

#include <linenoise.h>

#include "DragonSrv.h"

using namespace std;
using boost::format;
using namespace boost::algorithm;

namespace po = boost::program_options;

namespace
{
const unsigned short dhtBackBonePort = 11000;
const unsigned int consoleHintColor = 35;
};

map<string,string> consoleCmd = boost::assign::map_list_of
	("help","")
	("get"," <key>")
	("put"," <key> <value>")
	("status","")
	("remove"," <key>")
	("quit","");

void
completion(const char *buf, linenoiseCompletions *lc)
{
	if (buf[0] == 'h') {
		linenoiseAddCompletion(lc, "help");
	} else if (buf[0] == 'q') {
		linenoiseAddCompletion(lc, "quit");
	} else if (buf[0] == 'g') {
		linenoiseAddCompletion(lc, "get");
	} else if (buf[0] == 'p') {
		linenoiseAddCompletion(lc, "put");
	} else if (buf[0] == 's') {
		linenoiseAddCompletion(lc, "status");
	} else if (buf[0] == 'r') {
		linenoiseAddCompletion(lc, "remove");
	}
}

char *
hints(const char *buf, int *color, int *bold)
{
	*color = consoleHintColor;
	char* result = nullptr;

	if (consoleCmd.count(buf)) {
		result = const_cast<char*>(consoleCmd[buf].c_str());
	}

	return result;
}

int
main(int argc, const char *argv[])
{
	unsigned short inputPort;
	auto dhtPort = dhtBackBonePort;
	bool interactiveMode = false;

#if (1) // Cmd line parsing region
	po::options_description argumentsDescription{"Options"};
	argumentsDescription.add_options()
			("help,h", "Print help messages")
			("port,p", po::value<unsigned short>(&inputPort), "Node Communication port")
			("dht,d", po::value<unsigned short>(&dhtPort), "DHT Communication port")
			("interactive,i", "Enable interactive mode");

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
	unique_ptr<DragonStore::DragonSrv> spDragonSrv(
		new DragonStore::DragonSrv(io_service));

	if (!interactiveMode) {
		spDragonSrv->run();
	} else {
#if (1) // interactive mode

		cout << format("DHT node (id=%1%) is running on %2%:%3%\n") %
				spDragonSrv->getDhtId() % spDragonSrv->getIp() %
				spDragonSrv->getPort();
		cout << format("Waiting for requests on port %1%. Press CTRL-C "
			       "to "
			       "exit.\n") %
				spDragonSrv->getDragonPort();

		char *inLine = nullptr;
		linenoiseSetCompletionCallback(completion);
		linenoiseSetHintsCallback(hints);

		while ((inLine = linenoise("dragon> ")) != NULL) {
			std::string strLine(inLine);
			if (inLine[0] == '\0' || starts_with(strLine, "help")) {
				cout << "Following commands supported:" << endl;
				for (auto cmdEntry : consoleCmd) {
					cout << "\t- " << cmdEntry.first << cmdEntry.second << endl;
				}
			} else if (starts_with(strLine, "g")) {
			} else if (starts_with(strLine, "p")) {
			} else if (starts_with(strLine, "r")) {
			} else if (starts_with(strLine, "s")) {
				cout << spDragonSrv->getDhtPeerStatus();
			} else if (starts_with(strLine, "q")) {
				free(inLine);
				break;
			} else {
				cout << format("Unreconized command: %1%\n") % strLine;
				if (spDragonSrv->stopped()) {
					free(inLine);
					break;
				}
			}
			free(inLine);
		}

#endif
	}

	return 0;
}
