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

#include "../../include/dht/CChordNode.h"

#include <set>

#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/test/unit_test.hpp>
#include "../../include/dht/DhtUtils.h"

using namespace std;

namespace ut = boost::unit_test;

namespace
{
const string dhtBackBoneIp = "127.0.0.1";
const unsigned short reservedPortsEnd = 1024;
}

BOOST_AUTO_TEST_SUITE(DhtTests)

/**
 * @todo test disabled due cChord instability
 */
BOOST_AUTO_TEST_CASE(CChordNode_getPeerNodes, *ut::disabled())
{
	as::io_service io_service;
	unsigned short dhtPort = 0;

	FogKV::CChordAdapter *pDhtFirstNode =
		new FogKV::CChordAdapter(io_service, dhtPort, 0, true);
	dhtPort = pDhtFirstNode->getPort();

	BOOST_CHECK_EQUAL(dhtBackBoneIp, pDhtFirstNode->getIp());
	BOOST_CHECK_NE(pDhtFirstNode->getDhtId(), 0);

	boost::ptr_vector<FogKV::PureNode> peerNodes;
	BOOST_CHECK_EQUAL(pDhtFirstNode->getPeerList(peerNodes), 0);

	/*!
	 * Second Node ADDED
	 */
	FogKV::CChordAdapter *pDhtSecondNode =
		new FogKV::CChordAdapter(io_service, dhtPort, 0, true);
	sleep(1);

	pDhtFirstNode->refresh();
	pDhtSecondNode->refresh();

	BOOST_CHECK_NE(dhtPort, pDhtSecondNode->getPort());
	BOOST_CHECK_EQUAL(dhtBackBoneIp, pDhtSecondNode->getIp());
	BOOST_CHECK_NE(pDhtSecondNode->getDhtId(), 0);
	BOOST_CHECK_EQUAL(pDhtFirstNode->getPeerList(peerNodes), 1);
	BOOST_CHECK_EQUAL(peerNodes[0].getDhtId(), pDhtSecondNode->getDhtId());
	boost::ptr_vector<FogKV::PureNode> peerNodesSecond;
	BOOST_CHECK_EQUAL(pDhtSecondNode->getPeerList(peerNodesSecond), 1);
	BOOST_CHECK_EQUAL(peerNodesSecond[0].getDhtId(),
			  pDhtFirstNode->getDhtId());

	/*!
	 * Third Node ADDED
	 */
	FogKV::CChordAdapter *pDhtThirdNode =
		new FogKV::CChordAdapter(io_service, dhtPort, 0, true);
	sleep(1);

	BOOST_CHECK_NE(dhtPort, pDhtThirdNode->getPort());
	BOOST_CHECK_EQUAL(dhtBackBoneIp, pDhtThirdNode->getIp());
	BOOST_CHECK_NE(pDhtThirdNode->getDhtId(), 0);

	set<unsigned int> allFoundNodes;
	pDhtThirdNode->refresh();

	peerNodes.clear();
	pDhtFirstNode->refresh();
	BOOST_CHECK_GE(pDhtFirstNode->getPeerList(peerNodes), 1);
	for (auto node : peerNodes) {
		allFoundNodes.emplace(node.getDhtId());
	}
	peerNodesSecond.clear();
	pDhtSecondNode->refresh();
	BOOST_CHECK_GE(pDhtSecondNode->getPeerList(peerNodesSecond), 1);
	for (auto node : peerNodesSecond) {
		allFoundNodes.emplace(node.getDhtId());
	}
	boost::ptr_vector<FogKV::PureNode> peerNodesThird;
	BOOST_CHECK_GE(pDhtThirdNode->getPeerList(peerNodesThird), 1);
	for (auto node : peerNodesThird) {
		allFoundNodes.emplace(node.getDhtId());
	}

	BOOST_CHECK_NE(allFoundNodes.count(pDhtFirstNode->getDhtId()), 0);
	BOOST_CHECK_NE(allFoundNodes.count(pDhtSecondNode->getDhtId()), 0);
	BOOST_CHECK_NE(allFoundNodes.count(pDhtThirdNode->getDhtId()), 0);

	//! Intentionally left pDhtFirstNode, pDhtSecondNode and pDhtThirdNode -
	//! issue with cChord
}

BOOST_AUTO_TEST_SUITE_END()