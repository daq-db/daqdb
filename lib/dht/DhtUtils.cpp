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

#include "DhtUtils.h"

#include <iostream>

#include <asio/io_service.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/signal_set.hpp>

namespace DaqDB
{
namespace utils
{

unsigned short
getFreePort(asio::io_service &io_service, const unsigned short backbonePort, bool reuseAddr)
{
	auto resultPort = backbonePort;
	asio::ip::tcp::acceptor acceptor(io_service);
	std::error_code checkPortErrorCode;

	if (backbonePort) {
		acceptor.open(asio::ip::tcp::v4(), checkPortErrorCode);

		if (reuseAddr) {
			asio::socket_base::reuse_address option(true);
			acceptor.set_option(option);
		}

		acceptor.bind({asio::ip::tcp::v4(), resultPort}, checkPortErrorCode);
		acceptor.close();
	}
	if (!backbonePort || checkPortErrorCode) {
		acceptor.open(asio::ip::tcp::v4(), checkPortErrorCode);
		acceptor.bind({asio::ip::tcp::v4(), 0}, checkPortErrorCode);
		resultPort = acceptor.local_endpoint().port();
		acceptor.close();
	}

	return resultPort;
}

}
}
