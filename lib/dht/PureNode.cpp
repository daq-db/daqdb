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

#include "PureNode.h"

namespace FogKV
{

PureNode::PureNode() : _port(0), _dhtId(0), _dragonPort(0)
{
}

PureNode::PureNode(const std::string &ip, unsigned int dhtId,
		   unsigned short port, unsigned short dragonPort)
    : _port(port), _dragonPort(dragonPort), _dhtId(dhtId), _ip(ip)
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

unsigned short
PureNode::getDragonPort() const
{
	return _dragonPort;
}

void
PureNode::setDragonPort(unsigned short port)
{
	_dragonPort = port;
}

} /* namespace Dht */
