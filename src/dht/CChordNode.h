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

#ifndef DHT_CCHORTADAPTER_H_
#define DHT_CCHORTADAPTER_H_

#include "DhtNode.h"
#include <boost/asio/io_service.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

#include "ChordNode.h"

namespace as = boost::asio;

namespace Dht
{

class CChordAdapter : public Dht::DhtNode {
public:
	CChordAdapter(as::io_service &io_service, unsigned short port);
	CChordAdapter(as::io_service &io_service, unsigned short port,
		      bool skipShutDown);
	virtual ~CChordAdapter();

	std::string printStatus();
	void refresh();

	//! dragon required API
	unsigned int getPeerList(boost::ptr_vector<PureNode> &peerNodes);
	void triggerAggregationUpdate();

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

} /* namespace Dht */

#endif /* DHT_CCHORTADAPTER_H_ */
