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

#include "Node.h"
#include "debug.h"
#include <string.h>

using namespace Fabric;

Node::Node(const std::string &node, const std::string &serv) :
	mNode(new FabricNode(node, serv))
{
	mTxMR = mNode->registerMR(static_cast<void *>(mTxMsgBuff), MSG_BUFF_SIZE, SEND);
	mRxMR = mNode->registerMR(static_cast<void *>(mRxMsgBuff), MSG_BUFF_SIZE, RECV); 
}

Node::~Node()
{
}

void Node::onRecvHandler(FabricConnection &conn, std::shared_ptr<FabricMR> mr, size_t len)
{
	union {
		MsgHdr Hdr;
		MsgParams Params;
		MsgOp Write;
		MsgOp WriteResp;
		MsgOp Read;
		MsgOp ReadResp;
		MsgPut Put;
		MsgOp PutResp;
		MsgGet Get;
		MsgGetResp GetResp;
		uint8_t buff[MSG_BUFF_SIZE];
	} msg;

	memcpy(msg.buff, mr->getPtr(), len);
	conn.postRecv(mr);

//	LOG4CXX_INFO(benchDragon, "Received message from " + conn.getPeerStr() + " " + MsgToString(&msg.Hdr));

	switch (msg.Hdr.Type)
	{
	case MSG_WRITE:
		onMsgWrite(conn, &msg.Write);
		break;
	case MSG_WRITE_RESP:
		onMsgWriteResp(conn, &msg.WriteResp);
		break;
	case MSG_READ:
		onMsgRead(conn, &msg.Read);
		break;
	case MSG_READ_RESP:
		onMsgReadResp(conn, &msg.ReadResp);
		break;
	case MSG_PARAMS:
		onMsgParams(conn, &msg.Params);
		break;
	case MSG_READY:
		onMsgReady(conn);
		break;
	case MSG_PUT:
		onMsgPut(conn, &msg.Put);
		break;
	case MSG_PUT_RESP:
		onMsgPutResp(conn, &msg.PutResp);
		break;
	case MSG_GET:
		onMsgGet(conn, &msg.Get);
		break;
	case MSG_GET_RESP:
		onMsgGetResp(conn, &msg.GetResp);
		break;
	}
}

void Node::onMsgWrite(Fabric::FabricConnection &conn, MsgOp *msg)
{
	LOG4CXX_INFO(benchDragon, "onMsgWrite function not implemented");
}

void Node::onMsgWriteResp(Fabric::FabricConnection &conn, MsgOp *msg)
{
	LOG4CXX_INFO(benchDragon, "onMsgWriteResp function not implemented");
}

void Node::onMsgRead(Fabric::FabricConnection &conn, MsgOp *msg)
{
	LOG4CXX_INFO(benchDragon, "onMsgRead function not implemented");
}

void Node::onMsgReadResp(Fabric::FabricConnection &conn, MsgOp *msg)
{
	LOG4CXX_INFO(benchDragon, "onMsgReadResp function not implemented");
}

void Node::onMsgParams(Fabric::FabricConnection &conn, MsgParams *msg)
{
	LOG4CXX_INFO(benchDragon, "onMsgParams function not implemented");
}

void Node::onMsgReady(Fabric::FabricConnection &conn)
{
	LOG4CXX_INFO(benchDragon, "onMsgReady function not implemented");
}

void Node::onMsgPut(Fabric::FabricConnection &conn, MsgPut *msg)
{
	LOG4CXX_INFO(benchDragon, "onMsgPut function not implemented");
}

void Node::onMsgPutResp(Fabric::FabricConnection &conn, MsgOp *msg)
{
	LOG4CXX_INFO(benchDragon, "onMsgPutResp function not implemented");
}

void Node::onMsgGet(Fabric::FabricConnection &conn, MsgGet *msg)
{
	LOG4CXX_INFO(benchDragon, "onMsgGet function not implemented");
}

void Node::onMsgGetResp(Fabric::FabricConnection &conn, MsgGetResp *msg)
{
	LOG4CXX_INFO(benchDragon, "onMsgGetResp function not implemented");
}
