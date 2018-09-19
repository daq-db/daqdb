/**
 * Copyright 2017-2018 Intel Corporation.
 *
 * This software and the related documents are Intel copyrighted materials,
 * and your use of them is governed by the express license under which they
 * were provided to you (Intel OBL Internal Use License).
 * Unless the License provides otherwise, you may not use, modify, copy,
 * publish, distribute, disclose or transmit this software or the related
 * documents without Intel's prior written permission.
 *
 * This software and the related documents are provided as is, with no
 * express or implied warranties, other than those that are expressly
 * stated in the License.
 */

#ifndef FABRIC_NODE_HPP
#define FABRIC_NODE_HPP

#include <functional>
#include <string>
#include <mutex>
#include <thread>
#include <map>
#include <list>
#include <condition_variable>
#include <fabric/FabricConnection.h>
#include <fabric/FabricMR.h>

namespace Fabric {

class FabricNode {
public:
	using FabricConnectionEventHandler =
		std::function<void (std::shared_ptr<FabricConnection>)>;
public:
	FabricNode(const FabricAttributes &attr, const std::string &node = "localhost", const std::string &serv = "1234");
	FabricNode(const std::string &node = "localhost", const std::string &serv = "1234");
	virtual ~FabricNode();

	void listen();
	void stop();

	std::shared_ptr<FabricConnection> connection(const std::string &node, const std::string &serv);
	void disconnect(std::shared_ptr<FabricConnection> &conn);
	void connect(std::shared_ptr<FabricConnection> &conn);
	void connectAsync(std::shared_ptr<FabricConnection> &conn);
	void connectWait(std::shared_ptr<FabricConnection> &conn);

	void onConnectionRequest(FabricConnectionEventHandler handler);
	void onConnected(FabricConnectionEventHandler handler);
	void onDisconnected(FabricConnectionEventHandler handler);

	std::shared_ptr<FabricMR> registerMR(void *ptr, size_t size, uint64_t perm);
protected:

	FabricAttributes mAttr;
	Fabric mFabric;

	FabricConnectionEventHandler mConnReqHandler;
	FabricConnectionEventHandler mConnectedHandler;
	FabricConnectionEventHandler mDisconnectedHandler;
	std::map<struct fid_ep *, std::shared_ptr<FabricConnection> > mConnected;
	std::map<struct fid_ep *, std::shared_ptr<FabricConnection> > mConnecting;
	std::list<std::shared_ptr<FabricMR> > mMRs;
	std::mutex mConnLock;
	std::condition_variable mConnCond;
	struct fid_pep *mPep;

	std::thread mEqThread;
	bool mEqThreadRun;
	void eqThreadFunc();
};

}

#endif // FABRIC_NODE_HPP
