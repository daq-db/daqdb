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

#include <dht/PureNode.h>

namespace DaqDB
{

PureNode::PureNode() : _port(0), _dhtId(0)
{
}

PureNode::PureNode(const std::string &ip, unsigned int dhtId,
		   unsigned short port)
    : _port(port), _dhtId(dhtId), _ip(ip)
{
}

PureNode::~PureNode()
{
}

unsigned int
PureNode::getDhtId() const
{
	return _dhtId;
}

void
PureNode::setDhtId(unsigned int dhtId)
{
	_dhtId = dhtId;
}

const std::string &
PureNode::getIp() const
{
	return _ip;
}

void
PureNode::setIp(const std::string &ip)
{
	_ip = ip;
}

unsigned short
PureNode::getPort() const
{
	return _port;
}

void
PureNode::setPort(unsigned short port)
{
	_port = port;
}

}
