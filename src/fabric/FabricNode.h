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
#ifndef FABRIC_NODE_HPP
#define FABRIC_NODE_HPP

#include <FabricConnection.h>

#include <functional>
#include <string>
#include <mutex>
#include <thread>
#include <map>
#include <condition_variable>

namespace Fabric {

class FabricNode {
public:
	using FabricConnectionEventHandler =
		std::function<void (FabricConnection &)>;
public:
	FabricNode(const FabricAttributes &attr, const std::string &node = "localhost", const std::string &serv = "1234");
	virtual ~FabricNode();

	void listen();
	void stop();

	FabricConnection &connect(const std::string &addr, const std::string &serv);
	FabricConnection &connectAsync(const std::string &addr, const std::string &serv);
	void connectWait(FabricConnection &conn);

	void onConnectionRequest(FabricConnectionEventHandler handler);
	void onConnected(FabricConnectionEventHandler handler);
	void onDisconnected(FabricConnectionEventHandler handler);
protected:
	FabricAttributes mAttr;
	Fabric mFabric;

	FabricConnectionEventHandler mConnReqHandler;
	FabricConnectionEventHandler mConnectedHandler;
	FabricConnectionEventHandler mDisconnectedHandler;
	std::map<struct fid_ep *, FabricConnection *> mConnected;
	std::map<struct fid_ep *, FabricConnection *> mConnecting;
	std::mutex mConnLock;
	std::condition_variable mConnCond;
	struct fid_pep *mPep;

	std::thread mEqThread;
	bool mEqThreadRun;
};

}

#endif // FABRIC_NODE_HPP
