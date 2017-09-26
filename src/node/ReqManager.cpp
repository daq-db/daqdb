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

#include "ReqManager.h"
#include <iostream>

namespace DragonNode
{

ReqManager::ReqManager(boost::asio::io_service &io_service, short port)
    : _io_service(io_service),
      _socket(io_service, as::ip::udp::endpoint(as::ip::udp::v4(), port))
{
	_socket.async_receive_from(
		boost::asio::buffer(_data, max_length), _sender_endpoint,
		boost::bind(&ReqManager::handle_receive_from, this,
			    boost::asio::placeholders::error,
			    boost::asio::placeholders::bytes_transferred));
}

ReqManager::~ReqManager()
{
}

void
ReqManager::handle_receive_from(const boost::system::error_code &error,
				size_t bytes_recvd)
{
	if (!error && bytes_recvd > 0) {
		_socket.async_send_to(
			boost::asio::buffer(_data, bytes_recvd),
			_sender_endpoint,
			boost::bind(
				&ReqManager::handle_send_to, this,
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred));

		std::cout.write(_data, bytes_recvd);

	} else {
		_socket.async_receive_from(
			boost::asio::buffer(_data, max_length),
			_sender_endpoint,
			boost::bind(
				&ReqManager::handle_receive_from, this,
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred));
	}
}

void
ReqManager::handle_send_to(const boost::system::error_code &error,
			   size_t bytes_sent)
{
	_socket.async_receive_from(
		boost::asio::buffer(_data, max_length), _sender_endpoint,
		boost::bind(&ReqManager::handle_receive_from, this,
			    boost::asio::placeholders::error,
			    boost::asio::placeholders::bytes_transferred));
}

} /* namespace Node */
