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

#include <db/FogSrv.h>

#include <chrono>
#include <ctime>
#include <sstream>

#include <boost/algorithm/string/join.hpp>
#include <boost/format.hpp>

#include <libpmemobj/pool_base.h>

#include "../store/PmemKVStore.h"
#include "../dht/DhtUtils.h"

using namespace std;
using boost::format;

namespace as = boost::asio;

namespace
{
const unsigned short dhtBackBonePort = 11000;
const string pmemKvEngine = "kvtree";
const string pmemKvBasePath = "/dev/shm/fogkv";
};

namespace FogKV
{

FogSrv::FogSrv(as::io_service &io_service, const unsigned short nodeId)
    : _io_service(io_service), _nodeId(nodeId)
{
	unsigned short inputPort;
	auto dhtPort = dhtBackBonePort;
	bool interactiveMode = false;

	auto requestPort = FogKV::utils::getFreePort(_io_service, 0);
	this->_spReqManager.reset(
		new FogKV::SocketReqManager(_io_service, requestPort));
	this->_spDhtNode.reset(
		new FogKV::CChordAdapter(_io_service, dhtPort, requestPort, _nodeId));

	unsigned short dbNameSuffix = _nodeId;
	auto isTemporaryDb = false;
	if (_nodeId == 0) {
		dbNameSuffix = this->getDhtId();
		isTemporaryDb = true;
	}

	this->_spStore.reset(
		new FogKV::PmemKVStore(dbNameSuffix, isTemporaryDb));
}

FogSrv::~FogSrv()
{
}

void
FogSrv::run()
{
	_io_service.run();
}

unsigned int
FogSrv::getDhtId() const
{
	return _spDhtNode->getDhtId();
}

const std::string &
FogSrv::getIp() const
{
	return _spDhtNode->getIp();
}

unsigned short
FogSrv::getPort() const
{
	return _spDhtNode->getPort();
}

std::size_t
FogSrv::poll()
{
	return _io_service.poll();
}

bool
FogSrv::stopped() const
{
	return _io_service.stopped();
}

unsigned short
FogSrv::getDragonPort() const
{
	return _spDhtNode->getDragonPort();
}

std::string
FogSrv::getDhtPeerStatus() const
{
	stringstream result;
	boost::ptr_vector<FogKV::PureNode> peerNodes;
	chrono::time_point<chrono::system_clock> timestamp;

	timestamp = chrono::system_clock::now();
	auto currentTime = chrono::system_clock::to_time_t(timestamp);
	auto peerCount = _spDhtNode->getPeerList(peerNodes);

	if (!peerCount) {
		result << format("%1%\tno DHT peers\n") %
				std::ctime(&currentTime);
	} else {
		vector<string> peersID;
		for (auto node : peerNodes) {
			peersID.push_back(std::to_string(node.getDhtId()));
		}
		result << format("%1%\t[%2%] DHT "
				 "peers found, IDs [%3%]\n") %
				std::ctime(&currentTime) % peerCount %
				boost::algorithm::join(peersID, ",");
	}
	return result.str();
}

} /* namespace DragonNode */
