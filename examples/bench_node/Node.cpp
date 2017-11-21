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
#include <assert.h>
#include <chrono>

using namespace Fabric;

Node::Node(const std::string &node, const std::string &serv, size_t buffSize, size_t txBuffCount, size_t rxBuffCount, bool logMsg) :
	mNode(new FabricNode(node, serv)),
	mTxMsgBuff(txBuffCount),
	mRxMsgBuff(rxBuffCount),
	mTxMR(txBuffCount),
	mRxMR(rxBuffCount),
	mLogMsg(logMsg)
{
	mRxBuff = std::make_shared<RingBuffer>(buffSize);
	mRxBuffMR = mNode->registerMR(mRxBuff->get(), mRxBuff->size(), REMOTE_WRITE);

	for (int i = 0; i < mTxMsgBuff.size(); i++) {
		mTxMR[i] = mNode->registerMR(static_cast<void *>(mTxMsgBuff[i]), MSG_BUFF_SIZE, SEND);
		mTxMRs.push_back(mTxMR[i].get());
	}
	for (int i = 0; i < mRxMsgBuff.size(); i++) {
		mRxMR[i] = mNode->registerMR(static_cast<void *>(mRxMsgBuff[i]), MSG_BUFF_SIZE, RECV); 
	}
}

Node::~Node()
{
}

void Node::onSendHandler(FabricConnection &conn, FabricMR *mr, size_t len)
{
	notifyTxBuff(mr);
}

void Node::onRecvHandler(FabricConnection &conn, FabricMR *mr, size_t len)
{
	union {
		MsgHdr Hdr;
		MsgParams Params;
		MsgPut Put;
		MsgOp PutResp;
		MsgGet Get;
		MsgGetResp GetResp;
		uint8_t buff[MSG_BUFF_SIZE];
	} msg;

	memcpy(msg.buff, mr->getPtr(), len);
	conn.postRecv(mr);

	if (mLogMsg)
		LOG4CXX_INFO(benchDragon, "RECV: " + MsgToString(&msg.Hdr));

	switch (msg.Hdr.Type)
	{
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

void Node::postRecv()
{
	for (int i = 0; i < mRxMR.size(); i++)
		mConn->postRecv(mRxMR[i].get());
}

FabricMR *Node::getTxBuff()
{
	bool noTxBuff = false;
	std::chrono::high_resolution_clock::time_point t1 =
		std::chrono::high_resolution_clock::now();
	std::unique_lock<std::mutex> l(mTxMRsLock);
	mTxMRsCond.wait(l, [&]{
		if (mTxMRs.size() == 0)
			noTxBuff = true;
		return mTxMRs.size() > 0;
	});

	std::chrono::high_resolution_clock::time_point t2 =
		std::chrono::high_resolution_clock::now();


	mTxMsgUsageSum += mTxMRs.size();
	mTxMsgUsageCnt++;

	FabricMR *mr = mTxMRs.back();
	mTxMRs.pop_back();
	
	l.unlock();

	if (noTxBuff) {
		std::chrono::duration<double> time = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1);
		if (time.count() > 0.1)
			LOG4CXX_INFO(benchDragon, "waiting for tx buff took " + std::to_string(time.count()) + " seconds");
	}

	return mr;
}

void Node::notifyTxBuff(Fabric::FabricMR *mr)
{
	std::unique_lock<std::mutex> l(mTxMRsLock);
	mTxMRs.push_back(mr);
	l.unlock();
	mTxMRsCond.notify_all();
}

void Node::postSend(MsgHdr *hdrp)
{
	FabricMR *mr = getTxBuff();
	memcpy(mr->getPtr(), hdrp, hdrp->Size);

	if (mLogMsg)
		LOG4CXX_INFO(benchDragon, "SEND: " + MsgToString(hdrp));


	mConn->send(mr, hdrp->Size);

}

double Node::getAvgTxBuffUsage(bool reset)
{
	if (mTxUsageCnt == 0)
		return 0;

	double avg = (double)mTxUsageSum / (double)mTxUsageCnt / (double)mTxBuff->size();

	if (reset) {
		mTxUsageSum = 0;
		mTxUsageCnt = 0;
	}

	return avg;
}

double Node::getAvgRxBuffUsage(bool reset)
{
	if (mRxUsageCnt == 0)
		return 0;

	double avg = (double)mRxUsageSum / (double)mRxUsageCnt / (double)mRxBuff->size();

	if (reset) {
		mTxUsageSum = 0;
		mTxUsageCnt = 0;
	}

	return avg;
}

double Node::getAvgTxMsgUsage(bool reset)
{
	std::unique_lock<std::mutex> l(mTxMRsLock);

	if (mTxMsgUsageCnt == 0)
		return 0;

	double avg = 1.0 - (double)mTxMsgUsageSum / (double)mTxMsgUsageCnt / (double)mTxMR.size();

	if (reset) {
		mTxMsgUsageSum = 0;
		mTxMsgUsageCnt = 0;
	}

	return avg;
}

void Node::txBuffStat()
{
	auto o = mTxBuff->occupied();
	assert(o <= mTxBuff->size());
	mTxUsageSum += o;
	mTxUsageCnt++;
}

void Node::rxBuffStat()
{
	auto o = mRxBuff->occupied();
	assert(o <= mRxBuff->size());

	mRxUsageSum += o;
	mRxUsageCnt++;
}
