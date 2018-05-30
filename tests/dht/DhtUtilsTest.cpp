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
const unsigned short echoProtocolPort = 7; //! Echo Protocol

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

unsigned short getFreePortNumber(asio::io_service &io_service) {
	auto resultPort = getRandomPortNumber();
	while (!isPortFree(io_service, resultPort)) {
		resultPort = getRandomPortNumber();
	}
	return resultPort;
}
}
;

/**
 * Test if Dht::utils::getFreePort give free port as result when no default.
 */
BOOST_AUTO_TEST_CASE(GenRandomPort) {
	asio::io_service io_service;

	auto resultPort = FogKV::utils::getFreePort(io_service, 0);

	BOOST_CHECK_GT(resultPort, reservedPortsEnd);
	auto isFreeResult = isPortFree(io_service, resultPort);
	BOOST_CHECK(isFreeResult.value() == 0);
}

/**
 * Test if Dht::utils::getFreePort give free port as result when default is given but it is already used
 */
BOOST_AUTO_TEST_CASE(GenRandomPort_DefaultIsUsed) {
	asio::io_service io_service;

	auto portThatIsOpened = echoProtocolPort;

	auto resultPort = FogKV::utils::getFreePort(io_service, portThatIsOpened);

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
	auto resultPort = FogKV::utils::getFreePort(io_service, freePort, true);

	BOOST_CHECK_GT(resultPort, reservedPortsEnd);
	auto isFreeResult = isPortFree(io_service, resultPort);
	BOOST_CHECK(isFreeResult.value() == 0);
	BOOST_CHECK_EQUAL(resultPort, freePort);
}
