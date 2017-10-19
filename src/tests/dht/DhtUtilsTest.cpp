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

#include <DhtUtils.h>

#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/test/unit_test.hpp>

#include <limits>
#include <random>

namespace ut = boost::unit_test;
namespace as = boost::asio;

namespace
{
const unsigned short reservedPortsEnd = 1024;
const unsigned short echoProtocolPort = 7; //! Echo Protocol

bool
isPortFree(as::io_service &io_service, const unsigned short portToTest)
{
	as::ip::tcp::acceptor acceptor(io_service);
	boost::system::error_code checkPortErrorCode;

	acceptor.open(as::ip::tcp::v4(), checkPortErrorCode);
	acceptor.bind({as::ip::tcp::v4(), portToTest}, checkPortErrorCode);
	acceptor.close();
	return (checkPortErrorCode == 0);
}

unsigned short
getRandomPortNumber()
{
	std::random_device rd;

	auto gen = std::bind(std::uniform_int_distribution<>(
				     reservedPortsEnd + 1, USHRT_MAX),
			     std::default_random_engine(rd()));

	return gen();
}

unsigned short
getFreePortNumber(as::io_service &io_service)
{
	auto resultPort = getRandomPortNumber();
	while (!isPortFree(io_service, resultPort)) {
		resultPort = getRandomPortNumber();
	}
	return resultPort;
}
};

BOOST_AUTO_TEST_SUITE(DhtUtilsTests, *ut::label("Dht"))

BOOST_AUTO_TEST_CASE(
	DhtUtilsTest_GenRandomPort,
	*ut::description("Test if Dht::utils::getFreePort give free "
			 "port as result when no default.\n"))
{
	as::io_service io_service;

	auto resultPort = Dragon::utils::getFreePort(io_service, 0);

	BOOST_CHECK_GT(resultPort, reservedPortsEnd);
	BOOST_TEST(isPortFree(io_service, resultPort),
		   "resultPort shall be free");
}

BOOST_AUTO_TEST_CASE(
	DhtUtilsTest_GenRandomPort_DefaultIsUsed,
	*ut::description("Test if Dht::utils::getFreePort give free "
			 "port as result when default is given but it "
			 "is already used."))
{
	as::io_service io_service;

	auto portThatIsOpened = echoProtocolPort;

	auto resultPort = Dragon::utils::getFreePort(io_service, portThatIsOpened);

	BOOST_CHECK_GT(resultPort, reservedPortsEnd);
	BOOST_TEST(isPortFree(io_service, resultPort),
		   "resultPort shall be free");
	BOOST_CHECK_NE(resultPort, portThatIsOpened);
}

BOOST_AUTO_TEST_CASE(DhtUtilsTest_GenRandomPort_DefaultNotUsed,
		     *ut::description("Test if Dht::utils::getFreePort "
				      "give free port as result when"))
{
	as::io_service io_service;

	auto freePort = getFreePortNumber(io_service);

	auto resultPort = Dragon::utils::getFreePort(io_service, freePort);

	BOOST_CHECK_GT(resultPort, reservedPortsEnd);
	BOOST_TEST(isPortFree(io_service, resultPort),
		   "resultPort shall be free");
	BOOST_CHECK_EQUAL(resultPort, freePort);
}

BOOST_AUTO_TEST_SUITE_END()
