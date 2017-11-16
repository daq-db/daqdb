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
#ifndef FABRIC_CONNECTION_HPP
#define FABRIC_CONNECTION_HPP

#include <Fabric.h>
#include <FabricMR.h>
#include <functional>
#include <vector>
#include <thread>
#include <mutex>
#include <map>

#include <rdma/fi_endpoint.h>
#include <rdma/fi_cm.h>
#include <rdma/fi_errno.h>

namespace Fabric {

class FabricConnection {
	using FabricConnectionDataEventHandler = 
		std::function<void (FabricConnection &conn, std::shared_ptr<FabricMR> mr, size_t len)>;
public:
	FabricConnection(Fabric &fabric, struct fi_info *info);
	FabricConnection(Fabric &fabric, const std::string &node, const std::string &serv);
	virtual ~FabricConnection();

	void connectAsync();
	void send(const std::string &msg);

	void postRecv(std::shared_ptr<FabricMR> mr);
	void send(std::shared_ptr<FabricMR> mr, size_t len = 0);
	void write(std::shared_ptr<FabricMR> mr, size_t offset, size_t len, uint64_t raddr, uint64_t rkey);

	void onRecv(FabricConnectionDataEventHandler handler) { mRecvHandler = handler; }

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

	FabricConnectionDataEventHandler mRecvHandler;

	std::mutex mRecvPostedLock;
	std::map<FabricMR *, std::shared_ptr<FabricMR> > mRecvPosted;
};

}
#endif // FABRIC_CONNECTION_HPP
