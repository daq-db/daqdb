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

#include "Node.h"

#include <string>
#include <mutex>
#include <condition_variable>

#include "../../../include/fabric/FabricNode.h"

class AuxNode : public Node {
public:
	using GetHandler = std::function<void (const std::string &, const std::string &)>;
	AuxNode(const std::string &node, const std::string &serv,
		const std::string &remoteNode, const std::string &remoteServ,
		size_t buffSize, size_t txBuffCount, size_t rxBuffCount, bool logMsg);
	virtual ~AuxNode();

	void put(const std::string &key, const std::vector<char> &value);
	void get(const std::string &key, std::vector<char> &value);
	void start();

protected:
	void onRecvHandler(Fabric::FabricConnection &conn, Fabric::FabricMR *mr, size_t len);
	void onSendHandler(Fabric::FabricConnection &conn, Fabric::FabricMR *mr, size_t len);
	void onMsgParams(Fabric::FabricConnection &conn, MsgParams *msg);
	void onMsgPutResp(Fabric::FabricConnection &conn, MsgOp *msg);
	void onMsgGetResp(Fabric::FabricConnection &conn, MsgGetResp *msg);
	void onMsgReady(Fabric::FabricConnection &conn);

	void sendParams();
	bool flushWrBuff(size_t offset, size_t len);
	void notifyReady();
	void waitReady();

	std::mutex mReadyLock;
	bool mReady;
	std::condition_variable mReadyCond;

	std::function<void ()> mReadyHandler;

	void clrGet();
	size_t waitGet();
	void notifyGet(size_t valSize);

	size_t mValSize;
	std::mutex mGetLock;
	std::condition_variable mGetCond;
};
