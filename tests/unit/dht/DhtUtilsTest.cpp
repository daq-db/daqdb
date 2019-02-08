/**
 *  Copyright (c) 2019 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License. 
 */

#include "../../lib/dht/DhtUtils.h"

#include <asio/io_service.hpp>
#include <asio/ip/tcp.hpp>

#include <limits>
#include <random>

#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>

namespace ut = boost::unit_test;

namespace {
const unsigned short reservedPortsEnd = 1024;
const unsigned short sshProtocolPort = 22;

std::error_code isPortFree(asio::io_service &io_service,
		const unsigned short portToTest) {
	asio::ip::tcp::acceptor acceptor(io_service);
	std::error_code checkPortErrorCode;

	acceptor.open(asio::ip::tcp::v4(), checkPortErrorCode);
	acceptor.bind( { asio::ip::tcp::v4(), portToTest }, checkPortErrorCode);
	acceptor.close();
	return checkPortErrorCode;
}

unsigned short getRandomPortNumber() {
	std::random_device rd;

	auto gen = std::bind(
			std::uniform_int_distribution<>(reservedPortsEnd + 1, USHRT_MAX),
			std::default_random_engine(rd()));

	return gen();
}
}

/**
 * Test if Dht::utils::getFreePort give free port as result when no default.
 */
BOOST_AUTO_TEST_CASE(GenRandomPort) {
	asio::io_service io_service;

	auto resultPort = DaqDB::utils::getFreePort(io_service, 0);

	BOOST_CHECK_GT(resultPort, reservedPortsEnd);
	auto isFreeResult = isPortFree(io_service, resultPort);
	BOOST_CHECK(isFreeResult.value() == 0);
}

/**
 * Test if Dht::utils::getFreePort give free port as result when default is given but it is already used
 */
BOOST_AUTO_TEST_CASE(GenRandomPort_DefaultIsUsed) {
	asio::io_service io_service;

	auto portThatIsOpened = sshProtocolPort;

	auto resultPort = DaqDB::utils::getFreePort(io_service, portThatIsOpened, false);

	BOOST_CHECK_GT(resultPort, reservedPortsEnd);
	auto isFreeResult = isPortFree(io_service, resultPort);
	BOOST_CHECK(isFreeResult.value() == 0);
	BOOST_CHECK_NE(resultPort, portThatIsOpened);
}

/**
 * Test if Dht::utils::getFreePort give free port as result when default is given.
 */
BOOST_AUTO_TEST_CASE(GenRandomPort_DefaultNotUsed) {
	asio::io_service io_service;

	auto freePort = getRandomPortNumber();
	auto resultPort = DaqDB::utils::getFreePort(io_service, freePort, true);

	BOOST_CHECK_GT(resultPort, reservedPortsEnd);
	auto isFreeResult = isPortFree(io_service, resultPort);
	BOOST_CHECK(isFreeResult.value() == 0);
	BOOST_CHECK_EQUAL(resultPort, freePort);
}
