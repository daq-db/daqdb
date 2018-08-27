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

#ifndef FABRIC_CONNECTION_HPP
#define FABRIC_CONNECTION_HPP

#include <functional>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <map>

#include <rdma/fi_endpoint.h>
#include <rdma/fi_cm.h>
#include <rdma/fi_errno.h>
#include <fabric/Fabric.h>
#include <fabric/FabricMR.h>

namespace Fabric {

class FabricConnection {
	using FabricConnectionDataEventHandler = 
		std::function<void (FabricConnection &conn, FabricMR *mr, size_t len)>;
public:
	FabricConnection(Fabric &fabric, struct fi_info *info);
	FabricConnection(Fabric &fabric, const std::string &node, const std::string &serv);
	virtual ~FabricConnection();

	void connectAsync();
	void close();
	void send(const std::string &msg);

	void postRecv(FabricMR *mr);
	void send(FabricMR *mr, size_t len = 0);
	void write(FabricMR *mr, size_t offset, size_t len, uint64_t raddr, uint64_t rkey);

	void onRecv(FabricConnectionDataEventHandler handler) { mRecvHandler = handler; }
	void onSend(FabricConnectionDataEventHandler handler) { mSendHandler = handler; }

	std::string getPeerStr();
	std::string getNameStr();

	struct fid_ep *endpoint() { return mEp; }
protected:
	Fabric &mFabric;
	FabricInfo mInfo;

	struct fid_ep *mEp;
	struct fi_cq_attr mCqAttr;
	struct fid_cq *mTxCq;
	struct fid_cq *mRxCq;

	void createCq();

	std::thread mRxThread;
	bool mRxThreadRun;
	void rxThreadFunc();

	std::thread mTxThread;
	bool mTxThreadRun;
	void txThreadFunc();

	FabricConnectionDataEventHandler mRecvHandler;
	FabricConnectionDataEventHandler mSendHandler;

	std::vector<struct fi_cq_msg_entry> mRecvEntries;
	std::mutex mRecvLock;
	std::condition_variable mRecvCond;
};

}
#endif // FABRIC_CONNECTION_HPP
