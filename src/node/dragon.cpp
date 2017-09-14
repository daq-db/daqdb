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

#include <boost/program_options.hpp>

#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/bind.hpp>

#include <boost/filesystem.hpp>

#include "ChordNode.h"
#include "ProtocolSingleton.h"

using namespace std;

namespace po = boost::program_options;
namespace as = boost::asio;

namespace
{
const string dhtBackBoneIp = "127.0.0.1";
const unsigned short dhtBackBonePort = 11000;
const string dhtOverlayIdentifier = "chordTestBed";
const string rootDirectory = ".";
};

int
main(int argc, const char *argv[])
{
	as::io_service io_service;
	unsigned short inputPort;
	unsigned short dhtPort = dhtBackBonePort;

	po::options_description argumentsDescription{"Options"};
	argumentsDescription.add_options()("help,h", "Print help messages")(
		"port,p", po::value<unsigned short>(&inputPort),
		"Node Communication port")("dht,d",
					   po::value<unsigned short>(&dhtPort),
					   "DHT Communication port");
	;
	po::variables_map parsedArguments;
	try {
		po::store(po::parse_command_line(argc, argv,
						 argumentsDescription),
			  parsedArguments);

		if (parsedArguments.count("help")) {
			std::cout << argumentsDescription << endl;
			return 0;
		}

		po::notify(parsedArguments);
	} catch (po::error &parserError) {
		cerr << "Invalid arguments: " << parserError.what() << endl
		     << endl;
		std::cerr << argumentsDescription << std::endl;
		return -1;
	}

	as::signal_set signals(io_service, SIGINT, SIGTERM);
	signals.async_wait(
		boost::bind(&boost::asio::io_service::stop, &io_service));

	as::ip::tcp::acceptor acceptor(io_service);
	boost::system::error_code checkPortErrorCode;
	acceptor.open(as::ip::tcp::v4(), checkPortErrorCode);
	acceptor.bind({as::ip::tcp::v4(), dhtPort}, checkPortErrorCode);
	acceptor.close();
	if (checkPortErrorCode == as::error::address_in_use) {
		acceptor.open(as::ip::tcp::v4(), checkPortErrorCode);
		acceptor.bind({as::ip::tcp::v4(), 0}, checkPortErrorCode);
		dhtPort = acceptor.local_endpoint().port();
		acceptor.close();
	}

	boost::filesystem::path dir(boost::filesystem::current_path());
	dir /= ".chord";
	boost::filesystem::create_directory(dir);
	dir /= "data";
	boost::filesystem::create_directory(dir);

	//! @todo jradtke Move to class
	string backBone[] = {
		dhtBackBoneIp,
	};
	ChordNode *node = P_SINGLETON->initChordNode(
		dhtBackBoneIp, dhtPort, dhtOverlayIdentifier, rootDirectory);
	Node *chord = new Node(backBone[0], dhtBackBonePort);
	node->join(chord);
	cout << "\n" << node->printStatus();

	for (;;) {

		//! @todo jradtke Put daemon tasks here
		cout << "\n" << node->printStatus();
		sleep(1);

		io_service.poll();
		if (io_service.stopped()) {
			break;
		}
	}

	node->shutDown();
	delete node;
	delete chord;

	return 0;
}