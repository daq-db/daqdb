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

#include <iostream>

namespace FogKV
{

/*!
 * Class that stores DHT stored/shared data.
 */
class PureNode {
public:
	PureNode();
	PureNode(const std::string &ip, unsigned int dhtId, unsigned short port,
		 unsigned short dragonPort);
	virtual ~PureNode();

	/**
	 * 	@return DHT ID for this node
	 */
	unsigned int getDhtId() const;

	/**
	 * @return IP address for this node
	 */
	const std::string &getIp() const;

	/**
	 * @return Port number for this node
	 */
	unsigned short getPort() const;

	/**
	 * @return Port number for this node
	 */
	unsigned short getDragonPort() const;

protected:
	void setIp(const std::string &ip);
	void setDhtId(unsigned int dhtId);
	void setPort(unsigned short port);
	void setDragonPort(unsigned short port);

private:
	std::string _ip;
	unsigned int _dhtId;
	unsigned short _port;
	unsigned short _dragonPort;
};

}
