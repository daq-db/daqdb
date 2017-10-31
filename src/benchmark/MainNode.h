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
#ifndef MAIN_NODE_H
#define MAIN_NODE_H

#include "Node.h"
#include "RingBuffer.h"

#include <string>
#include <FabricNode.h>

class MainNode : public Node {
public:
	using WriteHandler = std::function<void (std::shared_ptr<RingBuffer>)>;

	MainNode(const std::string &node, const std::string &serv, size_t wrBuffSize);
	virtual ~MainNode();

	void start();
	void onWrite(WriteHandler handler);
protected:

	void onConnectionRequestHandler(std::shared_ptr<Fabric::FabricConnection> conn);
	void onConnectedHandler(std::shared_ptr<Fabric::FabricConnection> conn);
	void onDisconnectedHandler(std::shared_ptr<Fabric::FabricConnection> conn);
	void onRecvHandler(Fabric::FabricConnection &conn, std::shared_ptr<Fabric::FabricMR> mr, size_t len);
	void onMsgWrite(Fabric::FabricConnection &conn, MsgOp *msg);

	std::shared_ptr<RingBuffer> mRemoteWrBuff;
	std::shared_ptr<Fabric::FabricMR> mRemoteWrBuffMR;
	WriteHandler mWriteHandler;
};

#endif // MAIN_NODE_H
