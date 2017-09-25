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

#include "CChordNode.h"

#include "DhtUtils.h"

#include <boost/filesystem.hpp>
#include <iostream>

#include "ProtocolSingleton.h"

namespace bf = boost::filesystem;
using namespace std;

namespace
{
const string dhtBackBoneIp = "127.0.0.1";
const string dhtOverlayIdentifier = "chordTestBed";
const string rootDirectory = ".";
};

namespace Dht
{

CChordAdapter::CChordAdapter(as::io_service &io_service, unsigned short port)
    : Dht::DhtNode(io_service, port)
{
	/*!
	 * Workaround for cChord library issue.
	 * If following directory not exist then we see segmentation
	 * fault on shutdown (2+ node case)
	 */
	auto dir(boost::filesystem::current_path());
	dir /= ".chord";
	bf::create_directory(dir);
	dir /= "data";
	bf::create_directory(dir);

	auto dhtPort = Dht::utils::getFreePort(io_service, port);

	string backBone[] = {
		dhtBackBoneIp,
	};

	spNode.reset(P_SINGLETON->initChordNode(
		dhtBackBoneIp, dhtPort, dhtOverlayIdentifier, rootDirectory));
	spChord.reset(new Node(backBone[0], port));

	spNode->join(spChord.get());

	this->setPort(spNode->getThisNode()->getPort());
	this->setDhtId(spNode->getThisNode()->getId());
	this->setIp(spNode->getThisNode()->getIp());
}

CChordAdapter::~CChordAdapter()
{
	spNode->shutDown();
}

std::string
CChordAdapter::printStatus()
{
	return spNode->printStatus();
}

unsigned int
CChordAdapter::getPeerList(boost::ptr_vector<PureNode> &peerNodes)
{
	std::vector<Node *> nodeFingerTable;
	spNode->getPeerList(nodeFingerTable);
	for (auto pNode : nodeFingerTable) {
		peerNodes.push_back(
			new Dht::PureNode(pNode->getIp(), pNode->getId(),
					  pNode->getPort()));
	}
	return peerNodes.size();
}

void
CChordAdapter::triggerAggregationUpdate()
{
	//! @todo jradtke Discussion needed how to implement this ...
}

} /* namespace Dht */
