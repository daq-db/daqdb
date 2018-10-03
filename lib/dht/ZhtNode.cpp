/**
 * Copyright 2018 Intel Corporation.
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

#include <sstream>

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/assign/list_of.hpp>

#include <ConfHandler.h>
#include <Util.h>

#include "DhtUtils.h"
#include "ZhtNode.h"
#include <Logger.h>

#include "EpollServer.h"
#include "ip_server.h"

namespace DaqDB {

using zht_const = iit::datasys::zht::dm::Const;
using boost::format;

std::map<NodeState, std::string> NodeStateStr = boost::assign::map_list_of(
    NodeState::Ready, "Ready")(NodeState::NotResponding, "Not Responding");

ZhtNode::ZhtNode(asio::io_service &io_service, unsigned short port,
                 const std::string &confFile, const std::string &neighborsFile)
    : DaqDB::DhtNode(io_service, port), _confFile(confFile),
      _neighborsFile(neighborsFile) {
    setPort(DaqDB::utils::getFreePort(io_service, port, true));
    _thread = new std::thread(&ZhtNode::_ThreadMain, this);

    if (boost::filesystem::exists(_confFile) &&
        boost::filesystem::exists(_neighborsFile)) {
        _client.c.init(_confFile, _neighborsFile);
        _client.setInitized();
        _initNeighbors();
    } else {
        DAQ_DEBUG("Cannot initialize ZHT (Invalid configuration files)");
    }
}

ZhtNode::~ZhtNode() {}

void ZhtNode::_ThreadMain() {
    auto zhtPort = to_string(getPort());
    ConfHandler::initConf(_confFile, _neighborsFile);
    EpollServer zhtServer(zhtPort.c_str(), new IPServer());
    DAQ_DEBUG("ZHT server started on port " + zhtPort);
    zhtServer.serve();
}

void ZhtNode::_initNeighbors() {
    for (unsigned int index = 0; index < ConfHandler::NeighborVector.size();
         ++index) {
        auto entry = ConfHandler::NeighborVector.at(index);
        auto dhtNode =
            new PureNode(entry.name(), index, std::stoi(entry.value()));
        auto dhtNodeInfo = new DhtNodeInfo();
        if (_client.c.ping(index) ==
            zht_const::toInt(zht_const::ZSC_REC_SUCC)) {
            dhtNodeInfo->state = NodeState::Ready;
        } else {
            dhtNodeInfo->state = NodeState::NotResponding;
        }
        _neighbors[dhtNode] = dhtNodeInfo;
    }
}

std::string ZhtNode::printStatus() {
    std::stringstream result;

    if (_client.isInitized()) {
        result << "DHT: active";
    } else {
        result << "DHT: inactive";
    }

    return result.str();
}

std::string ZhtNode::printNeighbors() {
    std::stringstream result;

    if (_neighbors.size()) {
        for (auto neighbor : _neighbors) {

            if (_client.c.ping(neighbor.first->getDhtId()) ==
                zht_const::toInt(zht_const::ZSC_REC_SUCC)) {
                neighbor.second->state = NodeState::Ready;
            } else {
                neighbor.second->state = NodeState::NotResponding;
            }
            result << boost::str(boost::format("[%1%:%2%]: %3%") %
                                 neighbor.first->getIp() %
                                 to_string(neighbor.first->getPort()) %
                                 NodeStateStr[neighbor.second->state]);
        }
    } else {
        result << "No neighbors";
    }

    return result.str();
}

unsigned int ZhtNode::getPeerList(std::vector<PureNode *> &peerNodes) {
    return peerNodes.size();
}

} // namespace DaqDB
