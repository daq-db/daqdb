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
#include "AuxNode.h"
#include "debug.h"
#include <string.h>

using namespace Fabric;
using namespace std::placeholders;

AuxNode::AuxNode(const std::string &node, const std::string &serv, const std::string &remoteNode, const std::string &remoteServ, size_t wrBuffSize) :
	Node(node, serv)
{
	mConn = mNode->connection(remoteNode, remoteServ);

	mRdBuff = std::make_shared<RingBuffer>(wrBuffSize);
	mRdBuffMR = mNode->registerMR(mRdBuff->get(), mRdBuff->size(), REMOTE_WRITE);
}

AuxNode::~AuxNode()
{
}

void AuxNode::start()
{
	mConn->onRecv(std::bind(&AuxNode::onRecvHandler, this, _1, _2, _3));

	mConn->postRecv(mRxMR);

	mNode->connectAsync(mConn);
}

void AuxNode::onReady(std::function<void ()> handler)
{
	mReadyHandler = handler;
}

void AuxNode::onRecvHandler(FabricConnection &conn, std::shared_ptr<FabricMR> mr, size_t len)
{
	Node::onRecvHandler(conn, mr, len);
}

void AuxNode::onMsgReady(Fabric::FabricConnection &conn)
{
	notifyReady();
}

void AuxNode::onMsgReadResp(Fabric::FabricConnection &conn, MsgOp *msg)
{
	mRdBuff->notifyWrite(msg->Size);
}

void AuxNode::onMsgParams(FabricConnection &conn, MsgParams *msg)
{
	mRemoteWrBuffDesc = msg->WriteBuff;
	mWrBuff = std::make_shared<RingBuffer>(mRemoteWrBuffDesc.Size, std::bind(&AuxNode::flushWrBuff, this, _1, _2));
	mWrBuffMR = mNode->registerMR(mWrBuff->get(), mWrBuff->size(), WRITE);

	sendParams();
}

void AuxNode::sendParams()
{
	MsgParams *msg = static_cast<MsgParams *>(mTxMR->getPtr());
	msg->Hdr.Type = MSG_PARAMS;
	msg->Hdr.Size = sizeof(MsgParams);
	msg->WriteBuff.Size = mRdBuffMR->getSize();
	msg->WriteBuff.Addr = (uint64_t)(mRdBuffMR->getPtr());
	msg->WriteBuff.Key = mRdBuffMR->getRKey();

	mConn->send(mTxMR, msg->Hdr.Size);
}

void AuxNode::onMsgWriteResp(FabricConnection &conn, MsgOp *msg)
{
	mWrBuff->notifyRead(msg->Size);
}

bool AuxNode::flushWrBuff(size_t offset, size_t len)
{
	mConn->write(mWrBuffMR, offset, len, mRemoteWrBuffDesc.Addr, mRemoteWrBuffDesc.Key);
}

void AuxNode::write(const uint8_t *ptr, size_t len)
{
	mWrBuff->write(ptr, len);

	MsgOp *msg = static_cast<MsgOp *>(mTxMR->getPtr());
	msg->Hdr.Type = MSG_WRITE;
	msg->Hdr.Size = sizeof(MsgOp);
	msg->Size = len;

	mConn->send(mTxMR, msg->Hdr.Size);
}

void AuxNode::read(uint8_t *ptr, size_t len)
{
	MsgOp *msg = static_cast<MsgOp *>(mTxMR->getPtr());
	msg->Hdr.Type = MSG_READ;
	msg->Hdr.Size = sizeof(MsgOp);
	msg->Size = len;

	mConn->send(mTxMR, msg->Hdr.Size);

	size_t rd = 0;
	mRdBuff->read(len, [&] (const uint8_t *buff, size_t l) -> ssize_t {
		memcpy(&ptr[rd], buff, l);
		rd += l;
	});

	msg->Hdr.Type = MSG_READ_RESP;
	msg->Hdr.Size = sizeof(MsgOp);
	msg->Size = len;

	mConn->send(mTxMR, msg->Hdr.Size);
}

void AuxNode::notifyReady()
{
	if(mReadyHandler)
		mReadyHandler();
}
