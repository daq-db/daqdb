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

#include <FabricConnection.h>
#include <FabricInfo.h>
#include <rdma/fi_rma.h>
#include <string.h>
#include <iostream>

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
		throw std::string("fi_endpoint() failed");
	
	createCq();

	ret = fi_ep_bind(mEp, &mFabric.eq()->fid, 0);
	if (ret)
		throw std::string("fi_ep_bind(ep, eq) failed");

	ret = fi_enable(mEp);
	if (ret)
		throw std::string("fi_enable() failed");
}

FabricConnection::FabricConnection(Fabric &fabric, struct fi_info *info) :
	mFabric(fabric),
	mInfo(info)
{
	// TODO error handling
	
	int ret;

	ret = fi_endpoint(mFabric.domain(), info, &mEp, NULL);
	if (ret)
		throw std::string("fi_endpoint() failed");

	createCq();

	ret = fi_ep_bind(mEp, &mFabric.eq()->fid, 0);
	if (ret)
		throw std::string("fi_ep_bind(ep, eq) failed");

	ret = fi_enable(mEp);
	if (ret)
		throw std::string("fi_enable() failed");

	ret = fi_accept(mEp, NULL, 0);
	if (ret)
		throw std::string("fi_accept() failed");
}

FabricConnection::~FabricConnection()
{
	if (mEp)
		fi_close(&mEp->fid);
	if (mTxCq)
		fi_close(&mTxCq->fid);
	if (mRxCq) {
		mRxThreadRun = false;
		mRxThread.join();

		fi_close(&mRxCq->fid);
	}
}

void FabricConnection::createCq()
{
	int ret;

	mCqAttr.size = 0;
	mCqAttr.flags = 0;
	mCqAttr.format = FI_CQ_FORMAT_MSG;
	mCqAttr.wait_obj = FI_WAIT_UNSPEC;
	mCqAttr.signaling_vector = 0;
	mCqAttr.wait_cond = FI_CQ_COND_NONE;
	mCqAttr.wait_set = NULL;

	ret = fi_cq_open(mFabric.domain(), &mCqAttr, &mTxCq, NULL);
	if (ret)
		throw std::string("fi_cq_open() failed");

	ret = fi_ep_bind(mEp, &mTxCq->fid, FI_TRANSMIT);
	if (ret)
		throw std::string("fi_ep_bind(ep, eq) failed");

	ret = fi_cq_open(mFabric.domain(), &mCqAttr, &mRxCq, NULL);
	if (ret)
		throw std::string("fi_cq_open() failed");

	ret = fi_ep_bind(mEp, &mRxCq->fid, FI_RECV);
	if (ret)
		throw std::string("fi_ep_bind(ep, eq) failed");

	mRxThreadRun = true;
	mRxThread = std::thread(&FabricConnection::rxThreadFunc, this);
}

void FabricConnection::rxThreadFunc()
{
	struct fi_cq_msg_entry entry;
	while (mRxThreadRun) {
		ssize_t sret = fi_cq_sread(mRxCq, &entry,
				1, NULL, 100);
		if (sret == -FI_EAGAIN)
			continue;

		if (!(entry.flags & FI_RECV))
			throw std::string("invalid event received");

		std::shared_ptr<FabricMR> mr;
		{
			std::unique_lock<std::mutex> l(mRecvPostedLock);

			auto m = mRecvPosted.find(static_cast<FabricMR *>(entry.op_context));
			if (m == mRecvPosted.end())
				throw std::string("invalid context received");
			
			mr = m->second;

			mRecvPosted.erase(m);
		}

		mRecvHandler(*this, mr, entry.len);
	}
}

void FabricConnection::connectAsync()
{
	int ret = fi_connect(mEp, mInfo.info()->dest_addr , NULL, 0);
	if (ret)
		throw std::string("fi_connect() failed");
}

void FabricConnection::send(const std::string &msg)
{
	throw std::string("not implemented");
}

void FabricConnection::postRecv(std::shared_ptr<FabricMR> mr)
{
	int ret = fi_recv(mEp, mr->getPtr(), mr->getSize(), mr->getLKey(), 0, mr.get());
	if (ret)
		throw std::string("fi_recv failed");

	std::unique_lock<std::mutex> l(mRecvPostedLock);
	mRecvPosted[mr.get()] = mr;
}

void FabricConnection::send(std::shared_ptr<FabricMR> mr, size_t len)
{
	size_t l = len ? len : mr->getSize();
	int ret = fi_send(mEp, mr->getPtr(), l, mr->getLKey(), 0, mr.get());
	if (ret)
		std::cerr << std::string("fi_send failed");

	struct fi_cq_msg_entry entry;
	entry.op_context = (void *)0xdeadbeef;
	ssize_t sret;
	do {
		sret = fi_cq_sread(mTxCq, &entry,
				1, NULL, -1);
	} while (sret == -FI_EAGAIN);

	if (sret < 0)
		std::cerr << std::string("fi_cq_sread failed");

	if (!(entry.flags & FI_SEND))
		std::cerr << std::string("invalid event received");

	if (entry.op_context != mr.get())
		std::cerr << std::string("invalid context received ") << entry.op_context << "!=" << mr.get();
}

void FabricConnection::write(std::shared_ptr<FabricMR> mr, size_t offset, size_t len, uint64_t raddr, uint64_t rkey)
{
	void *ptr = &static_cast<uint8_t *>(mr->getPtr())[offset];
	int ret = fi_write(mEp, ptr, len, mr->getLKey(), 0, raddr + offset, rkey, mr.get());
	if (ret)
		throw std::string("fi_write failed");

	struct fi_cq_msg_entry entry;
	ssize_t sret = fi_cq_sread(mTxCq, &entry,
			1, NULL, -1);
	if (sret < 0)
		throw std::string("fi_cq_sread failed");

	if (!(entry.flags & FI_WRITE))
		throw std::string("invalid event received");

	if (entry.op_context != mr.get())
		throw std::string("invalid context received");

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
