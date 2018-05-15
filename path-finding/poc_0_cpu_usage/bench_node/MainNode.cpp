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

#include "MainNode.h"
#include "debug.h"
#include <string.h>

using namespace Fabric;
using namespace std::placeholders;

MainNode::MainNode(const std::string &node, const std::string &serv,
	size_t buffSize, size_t txBuffCount, size_t rxBuffCount, bool logMsg) :
	Node(node, serv, buffSize, txBuffCount, rxBuffCount, logMsg)
{
	mNode->onConnectionRequest(std::bind(&MainNode::onConnectionRequestHandler, this, std::placeholders::_1));
	mNode->onConnected(std::bind(&MainNode::onConnectedHandler, this, std::placeholders::_1));
	mNode->onDisconnected(std::bind(&MainNode::onDisconnectedHandler, this, std::placeholders::_1));
}

MainNode::~MainNode()
{
	if (mConn != nullptr) {
		mNode->disconnect(mConn);
		mConn = nullptr;
	}
}

void MainNode::start()
{
	mNode->listen();
}

void MainNode::onConnectionRequestHandler(std::shared_ptr<FabricConnection> conn)
{
#ifdef FOGKV_USE_LOG4CXX
	LOG4CXX_INFO(benchDragon, "Connection request from " + conn->getPeerStr());
#endif

	conn->onRecv(std::bind(&MainNode::onRecvHandler, this, _1, _2, _3));
	conn->onSend(std::bind(&MainNode::onSendHandler, this, _1, _2, _3));

	mConn = conn;

	postRecv();
}

void MainNode::onConnectedHandler(std::shared_ptr<FabricConnection> conn)
{
#ifdef FOGKV_USE_LOG4CXX
	LOG4CXX_INFO(benchDragon, "Connected with " + conn->getPeerStr());
#endif

	MsgBuffDesc buffDesc(
		mRxBuffMR->getSize(),
		(uint64_t)(mRxBuffMR->getPtr()),
		mRxBuffMR->getRKey()
	);

	MsgParams msgParams(buffDesc);
	postSend(msgParams);
}


void MainNode::onDisconnectedHandler(std::shared_ptr<FabricConnection> conn)
{
#ifdef FOGKV_USE_LOG4CXX
	LOG4CXX_INFO(benchDragon, "Disconnected with " + conn->getPeerStr());
	exit(0);
#endif
}

void MainNode::onRecvHandler(Fabric::FabricConnection &conn, Fabric::FabricMR *mr, size_t len)
{
	Node::onRecvHandler(conn, mr, len);
}

void MainNode::onSendHandler(Fabric::FabricConnection &conn, Fabric::FabricMR *mr, size_t len)
{
	if (mr != mTxBuffMR.get())
		Node::onSendHandler(conn, mr, len);
}

void MainNode::onMsgParams(Fabric::FabricConnection &conn, MsgParams *msg)
{
	mTxBuffDesc = msg->WriteBuff;
	mTxBuff = std::make_shared<RingBuffer>(mTxBuffDesc.Size, std::bind(&MainNode::flushWrBuff, this, _1, _2));
	mTxBuffMR = mNode->registerMR(mTxBuff->get(), mTxBuff->size(), WRITE);

	MsgHdr msgReady(MSG_READY, sizeof(MsgHdr));
	postSend(&msgReady);
}

void MainNode::onMsgPut(Fabric::FabricConnection &conn, MsgPut *msg)
{
	rxBuffStat();
	mRxBuff->notifyWrite(msg->KeySize + msg->ValSize);
	std::string key;
	key.reserve(msg->KeySize);

	std::vector<char> value(msg->ValSize);

	mRxBuff->read(msg->KeySize, [&] (const uint8_t *buff, size_t l) -> ssize_t {
		key.append((const char *)buff, l);
	});

	size_t c = 0;
	mRxBuff->read(msg->ValSize, [&] (const uint8_t *buff, size_t l) -> ssize_t {
		memcpy(&value[0], buff, l);
		c += l;
	});

	mPutHandler(key, value);

	MsgOp msgPutResp(MSG_PUT_RESP, msg->KeySize + msg->ValSize);
	postSend(msgPutResp);
}

void MainNode::onMsgGet(Fabric::FabricConnection &conn, MsgGet *msg)
{
	rxBuffStat();
	mRxBuff->notifyWrite(msg->KeySize);
	std::string key;
	key.reserve(msg->KeySize);

	mRxBuff->read(msg->KeySize, [&] (const uint8_t *buff, size_t l) -> ssize_t {
		key.append((const char *)buff, l);
	});

	std::vector<char> value;
	mGetHandler(key, value);

	txBuffStat();
	mTxBuff->write((uint8_t *)&value[0], value.size());

	MsgGetResp msgGetResp(msg->KeySize, value.size());
	postSend(msgGetResp);
}

void MainNode::onMsgGetResp(Fabric::FabricConnection &conn, MsgGetResp *msg)
{
	txBuffStat();
	mTxBuff->notifyRead(msg->ValSize);
}

bool MainNode::flushWrBuff(size_t offset, size_t len)
{
	mConn->write(mTxBuffMR.get(), offset, len, mTxBuffDesc.Addr, mTxBuffDesc.Key);
}

void MainNode::onPut(PutHandler handler)
{
	mPutHandler = handler;
}

void MainNode::onGet(GetHandler handler)
{
	mGetHandler = handler;
}
