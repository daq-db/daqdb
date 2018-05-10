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

#include "../../include/fabric/FabricNode.h"

#include <boost/test/unit_test.hpp>
#include <mutex>
#include <condition_variable>
#include <iostream>
#include "../../include/fabric/FabricAttributes.h"

namespace ut = boost::unit_test;

namespace
{
	std::string hostname = "127.0.0.1";
}

BOOST_AUTO_TEST_SUITE(FabricTests)

struct Node {
	Node() :
		node(nullptr),
		onConnectionRequest(false),
		onConnected(false)
	{}

	void wait(std::function<bool (void)> func)
	{
		std::unique_lock<std::mutex> l(lock);
		cond.wait(l, [&] { return func(); });
	}

	Fabric::FabricNode *node;
	bool onConnectionRequest;
	bool onConnected;
	bool onDisconnected;
	std::string nameStr;
	std::string peerStr;

	std::mutex lock;
	std::condition_variable cond;
};

static Node *
newNode(const Fabric::FabricAttributes &attr, const std::string &addr, const std::string &serv)
{
	Node *node = new Node();
	node->node = new Fabric::FabricNode(attr, addr, serv);

	node->node->onConnectionRequest([=] (std::shared_ptr<Fabric::FabricConnection> conn) -> void {
		std::unique_lock<std::mutex> lock(node->lock);

		BOOST_CHECK_EQUAL(node->onConnectionRequest, false);
		BOOST_CHECK_EQUAL(node->onConnected, false);

		node->onConnectionRequest = true;

		lock.unlock();
		node->cond.notify_one();
	});


	node->node->onConnected([=] (std::shared_ptr<Fabric::FabricConnection> conn) -> void {
		std::unique_lock<std::mutex> lock(node->lock);

		BOOST_CHECK_EQUAL(node->onConnected, false);

		node->onConnected = true;

		lock.unlock();
		node->cond.notify_one();
	});

	return node;
}

static void
deleteNode(Node *node)
{
	delete node->node;
	delete node;
}


/** @TODO jradtke This test hangs, needs investigation */
BOOST_AUTO_TEST_CASE(FabricNodeConnect, *boost::unit_test::disabled())
{
	Fabric::FabricAttributes attr;
	attr.setProvider("sockets");

	Node *node1 = newNode(attr, "127.0.0.1", "1234");
	Node *node2 = newNode(attr, "127.0.0.1", "1235");

	node1->node->listen();
	node2->node->listen();

	std::shared_ptr<Fabric::FabricConnection> conn = node2->node->connection("127.0.0.1", "1234");
	node2->node->connect(conn);

	node1->wait([&] { return node1->onConnected; });
	node2->wait([&] { return node2->onConnected; });

	BOOST_CHECK_EQUAL(node1->onConnected, true);
	BOOST_CHECK_EQUAL(node1->onConnectionRequest, true);

	BOOST_CHECK_EQUAL(node2->onConnected, true);
	BOOST_CHECK_EQUAL(node2->onConnectionRequest, false);

	deleteNode(node1);
	deleteNode(node2);
}

BOOST_AUTO_TEST_SUITE_END()
