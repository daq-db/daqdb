/**
 *  Copyright (c) 2019 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License. 
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
