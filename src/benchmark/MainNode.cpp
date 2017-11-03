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

	mRemoteWrBuff = std::make_shared<RingBuffer>(wrBuffSize, RingBuffer::RingBufferFlush());
	mRemoteWrBuffMR = mNode->registerMR(static_cast<void *>(mRemoteWrBuff->get()), wrBuffSize, REMOTE_WRITE);
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
}

void MainNode::onConnectedHandler(std::shared_ptr<FabricConnection> conn)
{
	LOG4CXX_INFO(benchDragon, "Connected with " + conn->getPeerStr());

	MsgParams *msg = static_cast<MsgParams *>(mTxMR->getPtr());
	msg->Hdr.Type = MSG_PARAMS;
	msg->Hdr.Size = sizeof(*msg);
	msg->WriteBuff.Size = mRemoteWrBuffMR->getSize();
	msg->WriteBuff.Addr = (uint64_t)(mRemoteWrBuffMR->getPtr());
	msg->WriteBuff.Key = mRemoteWrBuffMR->getRKey();

	conn->send(mTxMR, msg->Hdr.Size);
}

void MainNode::onDisconnectedHandler(std::shared_ptr<FabricConnection> conn)
{
	LOG4CXX_INFO(benchDragon, "Disconnected with " + conn->getPeerStr());
}

void MainNode::onRecvHandler(FabricConnection &conn, std::shared_ptr<FabricMR> mr, size_t len)
{
	union {
		MsgHdr Hdr;
		MsgOp Write;
		uint8_t buff[MSG_BUFF_SIZE];
	} msg;

	memcpy(msg.buff, mr->getPtr(), len);
	conn.postRecv(mr);

	std::string msgStr;
	switch (msg.Hdr.Type)
	{
	case MSG_WRITE:
		msgStr = msg.Write.toString();
		onMsgWrite(conn, &msg.Write);
		break;
	}

	LOG4CXX_INFO(benchDragon, "Received message from " + conn.getPeerStr() + " " + msgStr);
}

void MainNode::onMsgWrite(FabricConnection &conn, MsgOp *msg)
{
	mRemoteWrBuff->notifyWrite(msg->Size);
	size_t occupied = mRemoteWrBuff->occupied();

	if (!mWriteHandler)
		return;

	mWriteHandler(mRemoteWrBuff);

	if (occupied > mRemoteWrBuff->occupied()) {
		MsgOp *res = static_cast<MsgOp *>(mTxMR->getPtr());
		res->Hdr.Type = MSG_WRITE_RESP;
		res->Hdr.Size = sizeof(MsgOp);
		res->Size = occupied - mRemoteWrBuff->occupied();

		conn.send(mTxMR, res->Hdr.Size);
	}
}

void MainNode::onWrite(WriteHandler handler)
{
	mWriteHandler = handler;
}
