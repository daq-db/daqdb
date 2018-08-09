/**
 * Copyright 2017-2018 Intel Corporation.
 *
 * This software and the related documents are Intel copyrighted materials,
 * and your use of them is governed by the express license under which they
 * were provided to you (Intel OBL Internal Use License).
 * Unless the License provides otherwise, you may not use, modify, copy,
 * publish, distribute, disclose or transmit this software or the related
 * documents without Intel's prior written permission.
 *
 * This software and the related documents are provided as is, with no
 * express or implied warranties, other than those that are expressly
 * stated in the License.
 */

#include <iostream>
#include <set>

#include <dht/CChordNode.h>

#include "DhtUtils.h"

using namespace std;

namespace
{
const string dhtBackBoneIp = "127.0.0.1";
const string dhtOverlayIdentifier = "chordTestBed";
const string rootDirectory = ".";
};

namespace FogKV
{

CChordAdapter::CChordAdapter(asio::io_service &io_service, unsigned short port,
			     unsigned short dragonPort, int id)
    : CChordAdapter(io_service, port, dragonPort, id, false)
{
}

CChordAdapter::CChordAdapter(asio::io_service &io_service, unsigned short port,
                             unsigned short dragonPort, int id,
                             bool skipShutDown)
    : FogKV::DhtNode(io_service, port, dragonPort), skipShutDown(skipShutDown) {
    auto dhtPort = FogKV::utils::getFreePort(io_service, port, true);

    string backBone[] = {
        dhtBackBoneIp,
    };

    // @TODO jradtke Replace with new DHT library
    /*
        spNode.reset(P_SINGLETON->initChordNode(
                id, dhtBackBoneIp, dhtPort, dragonPort, dhtOverlayIdentifier,
                rootDirectory));
        spChord.reset(new Node(backBone[0], id, port, dragonPort));
        spNode->join(spChord.get());
         */

    this->setPort(dhtPort);
    this->setDragonPort(dragonPort);
    this->setDhtId(id);
    this->setIp(dhtBackBoneIp);
}

CChordAdapter::~CChordAdapter() {
    if (!skipShutDown) {
        // @TODO jradtke Replace with new DHT library
        // spNode->shutDown();
    }
}

std::string
CChordAdapter::printStatus()
{
    // @TODO jradtke Replace with new DHT library
	// return spNode->printStatus();

    return "";
}

unsigned int CChordAdapter::getPeerList(std::vector<PureNode *> &peerNodes) {
    // @TODO jradtke Replace with new DHT library
    /*
        std::set<unsigned int> addedDhtNodes;
        auto addUniqueNode = [&addedDhtNodes, &peerNodes](Node *pNodeToAdd) {
                if (!addedDhtNodes.count(pNodeToAdd->getId())) {
                        peerNodes.push_back(new FogKV::PureNode(
                                pNodeToAdd->getIp(), pNodeToAdd->getId(),
                                pNodeToAdd->getPort(),
                                pNodeToAdd->getDragonPort()));
                        addedDhtNodes.emplace(pNodeToAdd->getId());
                }
        };
        addedDhtNodes.emplace(this->getDhtId());

        addUniqueNode(spNode->getPredecessor());
        addUniqueNode(spNode->getSuccessor());

        vector<Node *> nodeFingerTable;
        spNode->getPeerList(nodeFingerTable);
        for (auto pNode : nodeFingerTable) {
                addUniqueNode(pNode);
        }
        */
    return peerNodes.size();
}

void CChordAdapter::refresh() {
    // @TODO jradtke Replace with new DHT library
    /*
        spNode->stabilize();
        spNode->fixFingersTable();
        spNode->checkPredecessor();
        */
}

void
CChordAdapter::triggerAggregationUpdate()
{
	//! @todo jradtke Discussion needed how to implement this ...
}

} /* namespace Dht */
