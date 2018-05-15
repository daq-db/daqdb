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

#pragma once

#include <string>
#include <atomic>

#include <fabric/FabricNode.h>
#include "Protocol.h"

#include "RingBuffer.h"

#define MSG_BUFF_SIZE 64

class Node {
public:
	Node(const std::string &node, const std::string &serv,
		size_t buffSize, size_t txBuffCount, size_t rxBuffCount, bool logMsg);
	virtual ~Node();

	virtual void start() = 0;

	double getAvgTxBuffUsage(bool reset = false);
	double getAvgRxBuffUsage(bool reset = false);
	double getAvgTxMsgUsage(bool reset = false);
protected:
	virtual void onRecvHandler(Fabric::FabricConnection &conn, Fabric::FabricMR *mr, size_t len);
	virtual void onSendHandler(Fabric::FabricConnection &conn, Fabric::FabricMR *mr, size_t len);
	virtual void onMsgParams(Fabric::FabricConnection &conn, MsgParams *msg);
	virtual void onMsgReady(Fabric::FabricConnection &conn);
	virtual void onMsgPut(Fabric::FabricConnection &conn, MsgPut *msg);
	virtual void onMsgPutResp(Fabric::FabricConnection &conn, MsgOp *msg);
	virtual void onMsgGet(Fabric::FabricConnection &conn, MsgGet *msg);
	virtual void onMsgGetResp(Fabric::FabricConnection &conn, MsgGetResp *msg);

	virtual void postRecv();
	virtual void postSend(MsgHdr *hdrp);

	template <class T>
	void postSend(T &msg) { postSend(&msg.Hdr); }

	Fabric::FabricMR *getTxBuff();
	void notifyTxBuff(Fabric::FabricMR *mr);

	std::mutex mTxMRsLock;
	std::condition_variable mTxMRsCond;
	std::vector<Fabric::FabricMR *> mTxMRs;

	std::shared_ptr<Fabric::FabricNode> mNode;
	std::shared_ptr<Fabric::FabricConnection> mConn;
	std::vector<uint8_t [MSG_BUFF_SIZE]> mTxMsgBuff;
	std::vector<uint8_t [MSG_BUFF_SIZE]> mRxMsgBuff;
	std::vector<std::shared_ptr<Fabric::FabricMR>> mTxMR;
	std::vector<std::shared_ptr<Fabric::FabricMR>> mRxMR;
	bool mLogMsg;

	std::shared_ptr<RingBuffer> mTxBuff;
	std::shared_ptr<Fabric::FabricMR> mTxBuffMR;
	MsgBuffDesc mTxBuffDesc;

	std::shared_ptr<RingBuffer> mRxBuff;
	std::shared_ptr<Fabric::FabricMR> mRxBuffMR;

	std::atomic<unsigned long long> mTxMsgUsageSum;
	std::atomic<unsigned long long> mTxMsgUsageCnt;

	std::atomic<unsigned long long> mTxUsageSum;
	std::atomic<unsigned long long> mTxUsageCnt;
	void txBuffStat();

	std::atomic<unsigned long long> mRxUsageSum;
	std::atomic<unsigned long long> mRxUsageCnt;
	void rxBuffStat();
};
