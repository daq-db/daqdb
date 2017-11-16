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

MainNode::MainNode(const std::string &node, const std::string &serv, size_t wrBuffSize) :
	Node(node, serv)
{
	mNode->onConnectionRequest(std::bind(&MainNode::onConnectionRequestHandler, this, std::placeholders::_1));
	mNode->onConnected(std::bind(&MainNode::onConnectedHandler, this, std::placeholders::_1));
	mNode->onDisconnected(std::bind(&MainNode::onDisconnectedHandler, this, std::placeholders::_1));

	mWrBuff = std::make_shared<RingBuffer>(wrBuffSize);
	mWrBuffMR = mNode->registerMR(static_cast<void *>(mWrBuff->get()), wrBuffSize, REMOTE_WRITE);
}

MainNode::~MainNode()
{
}

void MainNode::start()
{
	mNode->listen();
}

void MainNode::onConnectionRequestHandler(std::shared_ptr<FabricConnection> conn)
{
	LOG4CXX_INFO(benchDragon, "Connection request from " + conn->getPeerStr());

	conn->onRecv(std::bind(&MainNode::onRecvHandler, this, _1, _2, _3));
	conn->postRecv(mRxMR);

	mConn = conn;
}

void MainNode::onConnectedHandler(std::shared_ptr<FabricConnection> conn)
{
	LOG4CXX_INFO(benchDragon, "Connected with " + conn->getPeerStr());

	MsgParams *msg = static_cast<MsgParams *>(mTxMR->getPtr());
	msg->Hdr.Type = MSG_PARAMS;
	msg->Hdr.Size = sizeof(*msg);
	msg->WriteBuff.Size = mWrBuffMR->getSize();
	msg->WriteBuff.Addr = (uint64_t)(mWrBuffMR->getPtr());
	msg->WriteBuff.Key = mWrBuffMR->getRKey();

	conn->send(mTxMR, msg->Hdr.Size);
}


void MainNode::onDisconnectedHandler(std::shared_ptr<FabricConnection> conn)
{
	LOG4CXX_INFO(benchDragon, "Disconnected with " + conn->getPeerStr());
}

void MainNode::onRecvHandler(Fabric::FabricConnection &conn, std::shared_ptr<Fabric::FabricMR> mr, size_t len)
{
	Node::onRecvHandler(conn, mr, len);
}

void MainNode::onMsgParams(Fabric::FabricConnection &conn, MsgParams *msg)
{
	mRdBuffDesc = msg->WriteBuff;
	mRdBuff = std::make_shared<RingBuffer>(mRdBuffDesc.Size, std::bind(&MainNode::flushWrBuff, this, _1, _2));
	mRdBuffMR = mNode->registerMR(mRdBuff->get(), mRdBuff->size(), WRITE);

	MsgHdr *res = static_cast<MsgHdr *>(mTxMR->getPtr());
	res->Type = MSG_READY;
	res->Size = sizeof(MsgHdr);

	conn.send(mTxMR, res->Size);
}

void MainNode::onMsgWrite(FabricConnection &conn, MsgOp *msg)
{
	mWrBuff->notifyWrite(msg->Size);
	size_t occupied = mWrBuff->occupied();

	mWriteHandler(mWrBuff);

	if (occupied > mWrBuff->occupied()) {
		MsgOp *res = static_cast<MsgOp *>(mTxMR->getPtr());
		res->Hdr.Type = MSG_WRITE_RESP;
		res->Hdr.Size = sizeof(MsgOp);
		res->Size = occupied - mWrBuff->occupied();

		conn.send(mTxMR, res->Hdr.Size);
	}
}

void MainNode::onMsgPut(Fabric::FabricConnection &conn, MsgPut *msg)
{
	mWrBuff->notifyWrite(msg->KeySize + msg->ValSize);
	std::string key;
	key.reserve(msg->KeySize);

	std::vector<char> value(msg->ValSize);

	mWrBuff->read(msg->KeySize, [&] (const uint8_t *buff, size_t l) -> ssize_t {
		key.append((const char *)buff, l);
	});

	size_t c = 0;
	mWrBuff->read(msg->ValSize, [&] (const uint8_t *buff, size_t l) -> ssize_t {
		memcpy(&value[0], buff, l);
		c += l;
	});

	mPutHandler(key, value);

	MsgOp *res = static_cast<MsgOp *>(mTxMR->getPtr());
	res->Hdr.Type = MSG_PUT_RESP;
	res->Hdr.Size = sizeof(MsgOp);
	res->Size = msg->KeySize + msg->ValSize;

	conn.send(mTxMR, res->Hdr.Size);
}

void MainNode::onMsgGet(Fabric::FabricConnection &conn, MsgGet *msg)
{
	mWrBuff->notifyWrite(msg->KeySize);
	std::string key;
	key.reserve(msg->KeySize);
	mWrBuff->read(msg->KeySize, [&] (const uint8_t *buff, size_t l) -> ssize_t {
		key.append((const char *)buff, l);
	});

	std::vector<char> value;
	mGetHandler(key, value);

	mRdBuff->write((uint8_t *)&value[0], value.size());

	MsgGetResp *res = static_cast<MsgGetResp *>(mTxMR->getPtr());
	res->Hdr.Type = MSG_GET_RESP;
	res->Hdr.Size = sizeof(MsgGetResp);
	res->KeySize = msg->KeySize;
	res->ValSize = value.size();

	conn.send(mTxMR, res->Hdr.Size);
}

void MainNode::onMsgGetResp(Fabric::FabricConnection &conn, MsgGetResp *msg)
{
	mRdBuff->notifyRead(msg->ValSize);
}

void MainNode::onMsgRead(Fabric::FabricConnection &conn, MsgOp *msg)
{
	size_t occupied = mRdBuff->occupied();

	mReadHandler(mRdBuff, msg->Size);

	if (mRdBuff->occupied() > occupied) {
		MsgOp *res = static_cast<MsgOp *>(mTxMR->getPtr());
		res->Hdr.Type = MSG_READ_RESP;
		res->Hdr.Size = sizeof(MsgOp);
		res->Size = mRdBuff->occupied() - occupied;

		conn.send(mTxMR, res->Hdr.Size);
	}
}

void MainNode::onMsgReadResp(Fabric::FabricConnection &conn, MsgOp *msg)
{
	mRdBuff->notifyRead(msg->Size);
}

void MainNode::onWrite(WriteHandler handler)
{
	mWriteHandler = handler;
}

void MainNode::onRead(ReadHandler handler)
{
	mReadHandler = handler;
}

bool MainNode::flushWrBuff(size_t offset, size_t len)
{
	mConn->write(mRdBuffMR, offset, len, mRdBuffDesc.Addr, mRdBuffDesc.Key);
}

void MainNode::onPut(PutHandler handler)
{
	mPutHandler = handler;
}

void MainNode::onGet(GetHandler handler)
{
	mGetHandler = handler;
}
