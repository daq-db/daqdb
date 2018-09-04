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

#pragma once

#include <asio/io_service.hpp>
#include <dht/DhtNode.h>

namespace DaqDB
{

/*!
 * Adapter for cChord classes.
 */
class CChordAdapter : public DaqDB::DhtNode {
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

	// @TODO replace with new DHT implementation library
	// unique_ptr<ChordNode> spNode;
	//unique_ptr<Node> spChord;

	bool skipShutDown;
};

}
