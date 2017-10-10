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

#include <FabricNode.h>

namespace Fabric {

FabricNode::FabricNode(const FabricAttributes &attr, const std::string &node, const std::string &serv) :
	mAttr(attr),
	mFabric(attr, node, serv, true)
{
	int ret;

	ret = fi_passive_ep(mFabric.fabric(), mFabric.info(), &mPep, NULL);
	if (ret)
		throw std::string("fi_passive_ep() failed");

	ret = fi_pep_bind(mPep, &mFabric.eq()->fid, 0);
	if (ret)
		throw std::string("fi_pep_bind(pep, eq) failed");

}

FabricNode::~FabricNode()
{
	if (mEqThreadRun)
		stop();

	if (mPep)
		fi_close(&mPep->fid);
}

void FabricNode::listen()
{
	int ret;

	ret = fi_listen(mPep);
	if (ret)
		throw std::string("fi_listen() failed");

	mEqThreadRun = true;
	mEqThread = std::thread([&] () -> void {
		ssize_t sret;
		uint32_t event;
		struct fi_eq_cm_entry entry;

		while (mEqThreadRun) {
			sret = fi_eq_sread(mFabric.eq(), &event, &entry, sizeof(entry), 100, 0);
			if (sret == -FI_EAGAIN)
				continue;

			if (event == FI_CONNREQ) {
				FabricConnection *conn = new FabricConnection(mFabric, entry.info);
				if (mConnReqHandler)
					mConnReqHandler(*conn);

				{
					std::unique_lock<std::mutex> lock(mConnLock);
					mConnecting[conn->endpoint()] = conn;
				}
			} else if (event == FI_CONNECTED) {
				std::unique_lock<std::mutex> lock(mConnLock);
				auto c = mConnecting.find((struct fid_ep *)entry.fid);
				if (c == mConnecting.end())
					throw std::string("Invalid endpoint");

				struct fid_ep *ep = c->first;
				FabricConnection *conn = c->second;
				mConnecting.erase(c);
				mConnected[ep] = conn;

				lock.unlock();
				mConnCond.notify_all();
				
				if(mConnectedHandler)
					mConnectedHandler(*conn);

			} else if (event == FI_SHUTDOWN) {
				std::unique_lock<std::mutex> lock(mConnLock);
				auto c = mConnected.find((struct fid_ep *)entry.fid);
				if (c == mConnected.end())
					throw std::string("Invalid endpoint");

				mConnected.erase(c);

				lock.unlock();
				mConnCond.notify_all();

				if(mDisconnectedHandler)
					mDisconnectedHandler(*c->second);
			}
		}
	});
}

void FabricNode::stop()
{
	mEqThreadRun = false;
	mEqThread.join();
}

FabricConnection & FabricNode::connectAsync(const std::string &addr, const std::string &serv)
{
	std::unique_lock<std::mutex> lock(mConnLock);
	FabricConnection *conn = new FabricConnection(mFabric);
	conn->connect(addr, serv);
	mConnecting[conn->endpoint()] = conn;
	return *conn;
}

void FabricNode::connectWait(FabricConnection &conn)
{
	std::unique_lock<std::mutex> lock(mConnLock);
	mConnCond.wait(lock, [&] {
		return mConnecting.find(conn.endpoint()) == mConnecting.end();
	});
}

FabricConnection & FabricNode::connect(const std::string &addr, const std::string &serv)
{
	FabricConnection &conn = connectAsync(addr, serv);
	connectWait(conn);
	return conn;
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

}
