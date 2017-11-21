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

#pragma once

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/signal_set.hpp>

#include <dht/CChordNode.h>
#include <dht/DhtNode.h>
#include <dht/DhtUtils.h>
#include <store/KVStore.h>
#include "SocketReqManager.h"

namespace as = boost::asio;

namespace FogKV
{

/*!
 * Main node class, stores critical ingredients:
 * - KV store
 * - DHT
 * - External API manager
 */
class FogSrv {
public:
	FogSrv(as::io_service &io_service, const unsigned short nodeId = 0);
	virtual ~FogSrv();

	/*!
	 * Run the io_service object's event processing loop.
	 */
	void run();

	/*!
	 * Run the io_service object's event processing loop to execute ready
	 * handlers
	 * @return The number of handlers that were executed.
	 */
	std::size_t poll();

	/*!
	 * Determine whether the io_service object has been stopped.
	 * @return true if io_service object has been stopped.
	 */
	bool stopped() const;

	/*!
	 * 	@return DHT ID for this node
	 */
	unsigned int getDhtId() const;

	/*!
	 * @return IP address for this node
	 */
	const std::string &getIp() const;

	/*!
	 * @return Port number for this node
	 */
	unsigned short getPort() const;

	/*!
	 * @return Port number for this node
	 */
	unsigned short getDragonPort() const;

	/*!
	 * Example result:
	 * "Thu Oct 19 14:55:21 2017
	 *      no DHT peers"
	 *
	 * @return peer status string
	 */
	std::string getDhtPeerStatus() const;

	FogKV::KVStore *const
	getKvStore()
	{
		return _spStore.get();
	}

private:
	unsigned short _nodeId;
	as::io_service &_io_service;
	std::unique_ptr<FogKV::SocketReqManager> _spReqManager;
	std::unique_ptr<FogKV::DhtNode> _spDhtNode;

	std::unique_ptr<FogKV::KVStore> _spStore;
};

}
