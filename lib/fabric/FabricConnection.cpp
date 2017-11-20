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

#include "FabricConnection.h"
#include "FabricInfo.h"
#include <rdma/fi_rma.h>
#include <string.h>
#include <iostream>

#include "common.h"

#define CQ_SIZE 128

namespace Fabric {

FabricConnection::FabricConnection(Fabric &fabric, const std::string &node, const std::string &serv) :
	mFabric(fabric),
	mInfo(mFabric.attr(), node, serv, false),
	mEp(NULL),
	mTxCq(NULL),
	mRxCq(NULL)
{
	int ret;

	ret = fi_endpoint(mFabric.domain(), mInfo.info(), &mEp, NULL);
	if (ret)
		FATAL("fi_endpoint() failed");
	
	createCq();

	ret = fi_ep_bind(mEp, &mFabric.eq()->fid, 0);
	if (ret)
		FATAL("fi_ep_bind(ep, eq) failed");

	ret = fi_enable(mEp);
	if (ret)
		FATAL("fi_enable() failed");
}

FabricConnection::FabricConnection(Fabric &fabric, struct fi_info *info) :
	mFabric(fabric),
	mInfo(info)
{
	// TODO error handling
	
	int ret;

	ret = fi_endpoint(mFabric.domain(), info, &mEp, NULL);
	if (ret)
		FATAL("fi_endpoint() failed");

	createCq();

	ret = fi_ep_bind(mEp, &mFabric.eq()->fid, 0);
	if (ret)
		FATAL("fi_ep_bind(ep, eq) failed");

	ret = fi_enable(mEp);
	if (ret)
		FATAL("fi_enable() failed");

	ret = fi_accept(mEp, NULL, 0);
	if (ret)
		FATAL("fi_accept() failed");
}

FabricConnection::~FabricConnection()
{
	close();

	fi_close(&mEp->fid);

}

void FabricConnection::close()
{
	if (mRxCq) {
	}

	if (mTxCq) {
		mTxThreadRun = false;
		int ret = fi_cq_signal(mTxCq);
		if (ret)
			FATAL("fi_cq_signal failed " + std::to_string(ret));

		mTxThread.join();

		fi_close(&mTxCq->fid);
		mTxCq = NULL;

		mRxThreadRun = false;
		mRecvCond.notify_one();
		mRxThread.join();

	}

	fi_shutdown(mEp, 0);
}

void FabricConnection::createCq()
{
	int ret;

	mCqAttr.size = CQ_SIZE;
	mCqAttr.flags = 0;
	mCqAttr.format = FI_CQ_FORMAT_MSG;
	mCqAttr.wait_obj = FI_WAIT_UNSPEC;
	mCqAttr.signaling_vector = 0;
	mCqAttr.wait_cond = FI_CQ_COND_NONE;
	mCqAttr.wait_set = NULL;

	ret = fi_cq_open(mFabric.domain(), &mCqAttr, &mTxCq, NULL);
	if (ret)
		FATAL("fi_cq_open() failed");

	ret = fi_ep_bind(mEp, &mTxCq->fid, FI_TRANSMIT | FI_RECV);
	if (ret)
		FATAL("fi_ep_bind(ep, eq) failed");

	mRxThreadRun = true;
	mRxThread = std::thread(&FabricConnection::rxThreadFunc, this);

	mTxThreadRun = true;
	mTxThread = std::thread(&FabricConnection::txThreadFunc, this);
}

void FabricConnection::rxThreadFunc()
{
	while (mRxThreadRun) {
		struct fi_cq_msg_entry entry;
		std::unique_lock<std::mutex> l(mRecvLock);
		mRecvCond.wait(l, [&] { return mRecvEntries.size() > 0 || !mRxThreadRun; });
		if (!mRxThreadRun)
			break;

		entry = mRecvEntries.back();
		mRecvEntries.pop_back();
		l.unlock();

		FabricMR *mr = static_cast<FabricMR *>(entry.op_context);
		mRecvHandler(*this, mr, entry.len);
	}
}

void FabricConnection::txThreadFunc()
{
	struct fi_cq_msg_entry entry[CQ_SIZE];
	while (mTxThreadRun) {
		ssize_t sret = fi_cq_sread(mTxCq, entry,
				CQ_SIZE, NULL, -1);
		if (sret == -FI_EAGAIN || sret == -EINTR)
			continue;

		if (sret < 0)
			FATAL("fi_cq_sread failed" + std::to_string(sret));

		bool notify = false;
		for (size_t i = 0; i < (size_t)sret; i++) {
			if (entry[i].flags & FI_RECV) {
				std::unique_lock<std::mutex> l(mRecvLock);
				mRecvEntries.push_back(entry[i]);
				l.unlock();
				notify = true;
				continue;
			}
			if (!(entry[i].flags & FI_SEND) && !(entry[i].flags & FI_WRITE))
				FATAL("invalid event received");

			FabricMR *mr = static_cast<FabricMR *>(entry[i].op_context);
			mSendHandler(*this, mr, entry[i].len);
		}
		if (notify)
			mRecvCond.notify_one();
	}
}

void FabricConnection::connectAsync()
{
	int ret = fi_connect(mEp, mInfo.info()->dest_addr , NULL, 0);
	if (ret)
		FATAL("fi_connect() failed");
}

void FabricConnection::send(const std::string &msg)
{
	FATAL("not implemented");
}

void FabricConnection::postRecv(FabricMR *mr)
{
	int ret = fi_recv(mEp, mr->getPtr(), mr->getSize(), mr->getLKey(), 0, mr);
	if (ret)
		FATAL("fi_ecv failed");
}

void FabricConnection::send(FabricMR *mr, size_t len)
{
	size_t l = len ? len : mr->getSize();
	int ret = fi_send(mEp, mr->getPtr(), l, mr->getLKey(), 0, mr);
	if (ret)
		std::cerr << std::string("fi_send failed");
}

void FabricConnection::write(FabricMR *mr, size_t offset, size_t len, uint64_t raddr, uint64_t rkey)
{
	void *ptr = &static_cast<uint8_t *>(mr->getPtr())[offset];
	int ret = fi_write(mEp, ptr, len, mr->getLKey(), 0, raddr + offset, rkey, mr);
	if (ret)
		FATAL("fi_write failed");
}

std::string FabricConnection::getPeerStr()
{
	struct sockaddr_in addr;

	size_t addrlen = sizeof(addr);
	int ret = fi_getpeer(mEp, &addr, &addrlen);
	if (ret || addrlen != sizeof(addr))
		return "";

	return FabricInfo::addr2str(addr);
}

std::string FabricConnection::getNameStr()
{
	struct sockaddr_in addr;

	size_t addrlen = sizeof(addr);
	int ret = fi_getname(&mEp->fid, &addr, &addrlen);
	if (ret || addrlen != sizeof(addr))
		return "";

	return FabricInfo::addr2str(addr);
}

}
