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

#include <fabric/FabricNode.h>

#include "common.h"
namespace Fabric {

FabricNode::FabricNode(const FabricAttributes &attr, const std::string &node, const std::string &serv) :
	mAttr(attr),
	mFabric(attr, node, serv, true)
{
	int ret;

	ret = fi_passive_ep(mFabric.fabric(), mFabric.info(), &mPep, NULL);
	if (ret)
		FATAL("fi_passive_ep() failed");

	ret = fi_pep_bind(mPep, &mFabric.eq()->fid, 0);
	if (ret)
		FATAL("fi_pep_bind(pep, eq) failed");

	mEqThreadRun = true;
	mEqThread = std::thread(&FabricNode::eqThreadFunc, this);
}

FabricNode::FabricNode(const std::string &node, const std::string &serv) :
	FabricNode(FabricAttributes(), node, serv)
{
}

FabricNode::~FabricNode()
{
	if (mEqThreadRun)
		stop();

	if (mPep)
		fi_close(&mPep->fid);
}


void FabricNode::eqThreadFunc()
{
	uint32_t event;
	struct fi_eq_cm_entry entry;

	while (mEqThreadRun) {
		auto sret = fi_eq_sread(mFabric.eq(), &event, &entry, sizeof(entry), -1, 0);
		if (sret == -FI_EAGAIN)
			continue;

		if (sret == -EINTR)
			continue;

		if (sret < 0)
			FATAL("fi_eq_sread failed " + std::to_string(sret));

		if (event == FI_SHUTDOWN && entry.fid == &mPep->fid)
			break;

		if (event == FI_CONNREQ) {
			std::shared_ptr<FabricConnection> conn = std::make_shared<FabricConnection>(mFabric, entry.info);
			if (mConnReqHandler)
				mConnReqHandler(conn);

			{
				std::unique_lock<std::mutex> lock(mConnLock);
				mConnecting[conn->endpoint()] = conn;
			}
		} else if (event == FI_CONNECTED) {
			std::unique_lock<std::mutex> lock(mConnLock);
			auto c = mConnecting.find((struct fid_ep *)entry.fid);
			if (c == mConnecting.end())
				FATAL("Invalid endpoint");

			struct fid_ep *ep = c->first;
			std::shared_ptr<FabricConnection> conn = c->second;
			mConnecting.erase(c);
			mConnected[ep] = conn;

			lock.unlock();
			mConnCond.notify_all();
			
			if(mConnectedHandler)
				mConnectedHandler(conn);

		} else if (event == FI_SHUTDOWN) {
			std::unique_lock<std::mutex> lock(mConnLock);
			auto c = mConnected.find((struct fid_ep *)entry.fid);
			if (c == mConnected.end())
				FATAL("Invalid endpoint");

			if(mDisconnectedHandler)
				mDisconnectedHandler(c->second);

			mConnected.erase(c);

			lock.unlock();
			mConnCond.notify_all();
		}
	}
}

void FabricNode::listen()
{
	int ret;

	ret = fi_listen(mPep);
	if (ret)
		FATAL("fi_listen() failed");

}

void FabricNode::stop()
{
	mEqThreadRun = false;
	struct fi_eq_cm_entry entry;
	entry.info = NULL;
	entry.fid = &mPep->fid;
	ssize_t sret = fi_eq_write(mFabric.eq(), FI_SHUTDOWN, &entry, sizeof(entry), 0);
	if (sret < 0)
		FATAL("fi_eq_write failed");

	mEqThread.join();
}

std::shared_ptr<FabricConnection> FabricNode::connection(const std::string &addr, const std::string &serv)
{
	std::unique_lock<std::mutex> lock(mConnLock);
	std::shared_ptr<FabricConnection> conn = std::make_shared<FabricConnection>(mFabric, addr, serv);
	return conn;
}

void FabricNode::disconnect(std::shared_ptr<FabricConnection> conn)
{
	conn->close();
	std::unique_lock<std::mutex> lock(mConnLock);
	mConnCond.wait(lock, [&] {
		return mConnecting.find(conn->endpoint()) == mConnecting.end() &&
			mConnected.find(conn->endpoint()) == mConnected.end();
	});
}

void FabricNode::connectAsync(std::shared_ptr<FabricConnection> conn)
{
	std::unique_lock<std::mutex> lock(mConnLock);
	conn->connectAsync();
	mConnecting[conn->endpoint()] = conn;
}

void FabricNode::connectWait(std::shared_ptr<FabricConnection> conn)
{
	std::unique_lock<std::mutex> lock(mConnLock);
	mConnCond.wait(lock, [&] {
		return mConnecting.find(conn->endpoint()) == mConnecting.end();
	});
}

void FabricNode::connect(std::shared_ptr<FabricConnection> &conn)
{
	connectAsync(conn);
	connectWait(conn);
}

void FabricNode::onConnectionRequest(FabricConnectionEventHandler handler)
{
	mConnReqHandler = handler;
}

void FabricNode::onConnected(FabricConnectionEventHandler handler)
{
	mConnectedHandler = handler;
}

void FabricNode::onDisconnected(FabricConnectionEventHandler handler)
{
	mDisconnectedHandler = handler;
}

std::shared_ptr<FabricMR> FabricNode::registerMR(void *ptr, size_t size, uint64_t perm)
{
	std::shared_ptr<FabricMR> mr = std::make_shared<FabricMR>(mFabric, ptr, size, perm);

	mMRs.push_back(mr);

	return mr;
}

}
