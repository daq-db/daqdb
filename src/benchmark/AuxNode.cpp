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

AuxNode::AuxNode(const std::string &node, const std::string &serv, const std::string &remoteNode, const std::string &remoteServ,
	size_t buffSize, size_t txBuffCount, size_t rxBuffCount, bool logMsg) :
	Node(node, serv, buffSize, txBuffCount, rxBuffCount, logMsg)
{
	mConn = mNode->connection(remoteNode, remoteServ);
}

AuxNode::~AuxNode()
{
	if (mConn != nullptr) {
		mNode->disconnect(mConn);
		mConn = nullptr;
	}
}

void AuxNode::start()
{
	mConn->onRecv(std::bind(&AuxNode::onRecvHandler, this, _1, _2, _3));
	mConn->onSend(std::bind(&AuxNode::onSendHandler, this, _1, _2, _3));

	postRecv();

	mNode->connectAsync(mConn);

	waitReady();
}

void AuxNode::onRecvHandler(FabricConnection &conn, FabricMR *mr, size_t len)
{
	Node::onRecvHandler(conn, mr, len);
}

void AuxNode::onSendHandler(FabricConnection &conn, FabricMR *mr, size_t len)
{
	if (mr != mTxBuffMR.get())
		Node::onSendHandler(conn, mr, len);
}

void AuxNode::onMsgReady(Fabric::FabricConnection &conn)
{
	notifyReady();
}

void AuxNode::onMsgParams(FabricConnection &conn, MsgParams *msg)
{
	mTxBuffDesc = msg->WriteBuff;
	mTxBuff = std::make_shared<RingBuffer>(mTxBuffDesc.Size, std::bind(&AuxNode::flushWrBuff, this, _1, _2));
	mTxBuffMR = mNode->registerMR(mTxBuff->get(), mTxBuff->size(), WRITE);

	sendParams();
}

void AuxNode::sendParams()
{
	MsgBuffDesc desc(
		mRxBuffMR->getSize(),
		(uint64_t)(mRxBuffMR->getPtr()),
		mRxBuffMR->getRKey()
	);

	MsgParams msg(desc);
	postSend(msg);
}

bool AuxNode::flushWrBuff(size_t offset, size_t len)
{
	mConn->write(mTxBuffMR.get(), offset, len, mTxBuffDesc.Addr, mTxBuffDesc.Key);
}

void AuxNode::put(const std::string &key, const std::vector<char> &value)
{
	txBuffStat();
	mTxBuff->write((uint8_t *)key.c_str(), key.length());
	mTxBuff->write((uint8_t *)&value[0], value.size());

	MsgPut msgPut(key.length(), value.size());
	postSend(msgPut);
}

void AuxNode::get(const std::string &key, std::vector<char> &value)
{
	txBuffStat();
	mTxBuff->write((uint8_t *)key.c_str(), key.length());

	MsgGet msgGet(key.length());

	clrGet();

	postSend(msgGet);

	size_t valSize = waitGet();

	value.resize(valSize);
	size_t c = 0;
	rxBuffStat();
	mRxBuff->read(valSize, [&] (const uint8_t *buff, size_t l) -> ssize_t {
		memcpy(&value[c], buff, l);
		c += l;
	});
	
	MsgGetResp msgGetResp(key.length(), valSize);
	postSend(msgGetResp);
}

void AuxNode::onMsgGetResp(Fabric::FabricConnection &conn, MsgGetResp *msg)
{
	std::string key;
	key.reserve(msg->KeySize);

	std::string value;
	value.reserve(msg->ValSize);

	txBuffStat();
	mTxBuff->notifyRead(msg->KeySize);

	rxBuffStat();
	mRxBuff->notifyWrite(msg->ValSize);

	notifyGet(msg->ValSize);
}

void AuxNode::onMsgPutResp(Fabric::FabricConnection &conn, MsgOp *msg)
{
	txBuffStat();
	mTxBuff->notifyRead(msg->Size);
}

size_t AuxNode::waitGet()
{
	std::unique_lock<std::mutex> l(mGetLock);
	mGetCond.wait(l, [&] { return mValSize > 0; });

	return mValSize;
}

void AuxNode::notifyGet(size_t valSize)
{
	std::unique_lock<std::mutex> l(mGetLock);
	mValSize = valSize;
	l.unlock();
	mGetCond.notify_one();
}

void AuxNode::clrGet()
{
	std::unique_lock<std::mutex> l(mGetLock);
	mValSize = 0;
}

void AuxNode::notifyReady()
{
	std::unique_lock<std::mutex> l(mReadyLock);
	mReady = true;
	l.unlock();
	mReadyCond.notify_one();
}

void AuxNode::waitReady()
{
	std::unique_lock<std::mutex> l(mReadyLock);
	mReadyCond.wait(l, [&] { return mReady; });
}
