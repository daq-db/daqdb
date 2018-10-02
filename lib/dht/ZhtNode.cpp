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


#include <future>

#include "ZhtNode.h"
#include "DhtUtils.h"
#include <Logger.h>

#include "EpollServer.h"
#include "ip_server.h"

namespace DaqDB {

ZhtNode::ZhtNode(asio::io_service &io_service, unsigned short port)
    : DaqDB::DhtNode(io_service, port) {
    setPort(DaqDB::utils::getFreePort(io_service, port, true));
    _thread = new std::thread(&ZhtNode::_ThreadMain, this);
}

ZhtNode::~ZhtNode() {}

void ZhtNode::_ThreadMain() {
    auto zhtPort = to_string(getPort());
    EpollServer zhtServer(zhtPort.c_str(), new IPServer());
    FOG_DEBUG("Started zhtServer on port " + zhtPort) ;
    zhtServer.serve();
}

std::string ZhtNode::printStatus() { return ""; }

unsigned int ZhtNode::getPeerList(std::vector<PureNode *> &peerNodes) {
    return peerNodes.size();
}

}
