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

#include "DhtTestFixture.hpp"
#include <CChordNode.h>
#include <DhtUtils.h>

#include <boost/test/unit_test.hpp>

using namespace std;

namespace ut = boost::unit_test;

namespace
{
const string dhtBackBoneIp = "127.0.0.1";
const unsigned short reservedPortsEnd = 1024;
const unsigned short echoProtocolPort = 7; //! Echo Protocol
}

BOOST_FIXTURE_TEST_SUITE(DhtTests, DhtTestFixture)

BOOST_AUTO_TEST_CASE(CChordNode_BasicTwoNodes, *ut::description(""))
{
	BOOST_CHECK_EQUAL(dhtPort, pDhtNode->getPort());
	BOOST_CHECK_EQUAL(dhtBackBoneIp, pDhtNode->getIp());
	BOOST_CHECK_NE(pDhtNode->getDhtId(), 0);

	Dht::DhtNode* pDhtNodePeer = new Dht::CChordAdapter(io_service, dhtPort);

	BOOST_CHECK_NE(dhtPort, pDhtNodePeer->getPort());
	BOOST_CHECK_EQUAL(dhtBackBoneIp, pDhtNodePeer->getIp());
	BOOST_CHECK_NE(pDhtNodePeer->getDhtId(), 0);

	// @todo jradtke intentionaly not calling delete on pDhtNodePeer (there is a bug in cChort that broke ut)
}

BOOST_AUTO_TEST_SUITE_END()
