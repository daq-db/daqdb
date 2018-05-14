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

#include <asio/io_service.hpp>
#include <dht/DhtNode.h>
#include <ChordNode.h>

namespace FogKV
{

/*!
 * Adapter for cChord classes.
 */
class CChordAdapter : public FogKV::DhtNode {
public:
	CChordAdapter(asio::io_service &io_service, unsigned short port,
		      unsigned short dragonPort, int id);
	CChordAdapter(asio::io_service &io_service, unsigned short port,
		      unsigned short dragonPort, int id, bool skipShutDown);
	virtual ~CChordAdapter();

	/*!
	 * @return Status of the Node - format determined by 3rd party lib
	 */
	std::string printStatus();

	/*!
	 * Refresh internal DHT data (fingertable, succ, pred)
	 */
	void refresh();

	/*!
	 * Fill peerNodes vector with peer node list from DHT.
	 * This is a subset of full list of nodes in system.
	 *
	 * @param peerNodes vector to insert peer nodes
	 * @return number of peer nodes
	 */
	unsigned int getPeerList(std::vector<PureNode*> &peerNodes);

	/*!
	 * Triggers dragon aggregation table update.
	 * @todo jradtke triggerAggregationUpdate not implemented
	 */
	void triggerAggregationUpdate();

	/*!
	 * Workaround on cChord bug for unit tests
	 * @param skipShutDown
	 */
	void
	setSkipShutDown(bool skipShutDown)
	{
		this->skipShutDown = skipShutDown;
	}

private:
	unique_ptr<ChordNode> spNode;
	unique_ptr<Node> spChord;
	bool skipShutDown;
};

}
