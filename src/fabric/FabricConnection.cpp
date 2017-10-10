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

#include <string.h>

namespace Fabric {

FabricConnection::FabricConnection(Fabric &fabric) :
	mFabric(fabric),
	mInfo(NULL),
	mEp(NULL),
	mTxCq(NULL),
	mRxCq(NULL)
{
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
	initMem();

	ret = fi_ep_bind(mEp, &mFabric.eq()->fid, 0);
	if (ret)
		throw std::string("fi_ep_bind(ep, eq) failed");

	ret = fi_enable(mEp);
	if (ret)
		throw std::string("fi_enable() failed");

	postRecv();

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
	if (mRxCq)
		fi_close(&mRxCq->fid);
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
	mRxThread = std::thread([&] () -> void {
		try {
		struct fi_cq_msg_entry entry;
		while (mRxThreadRun) {
			ssize_t sret = fi_cq_sread(mRxCq, &entry,
					1, NULL, 100);
			if (sret == -FI_EAGAIN)
				continue;

			if (!(entry.flags & FI_RECV))
				throw std::string("invalid event received");

			if (mRecvHandler) {
				uint64_t *len = (uint64_t *)mRxBuff;
				std::vector<uint8_t> msg(*len);
				memcpy(msg.data(), (uint8_t *)(len + 1), *len);
				postRecv();
				mRecvHandler(msg);
			}
		}
		} catch (std::string &str) {
			printf("ERR: %s\n", str.c_str());
		}

	});

}

void FabricConnection::initMem()
{
	mBuffSize = mFabric.attr().mBufferSize;
	mTxBuff = new uint8_t[mBuffSize];
	mRxBuff = new uint8_t[mBuffSize];

	int ret;

	ret = fi_mr_reg(mFabric.domain(), mTxBuff, mBuffSize, FI_SEND, 0, 0, 0, &mTxMr, NULL);
	if (ret)
		throw std::string("fi_mr_reg() failed");

	ret = fi_mr_reg(mFabric.domain(), mRxBuff, mBuffSize, FI_RECV, 0, 0, 0, &mRxMr, NULL);
	if (ret)
		throw std::string("fi_mr_reg() failed");

}

void FabricConnection::postRecv()
{
	ssize_t sret;

	sret = fi_recv(mEp, mRxBuff, mBuffSize, fi_mr_desc(mRxMr), 0, NULL);
	if (sret < 0)
		throw std::string("fi_recv() failed");
}

void FabricConnection::connect(const std::string &node, const std::string &serv)
{
	int ret;

	ret = fi_endpoint(mFabric.domain(), mFabric.info(), &mEp, NULL);
	if (ret)
		throw std::string("fi_endpoint() failed");
	
	createCq();
	initMem();

	ret = fi_ep_bind(mEp, &mFabric.eq()->fid, 0);
	if (ret)
		throw std::string("fi_ep_bind(ep, eq) failed");

	ret = fi_enable(mEp);
	if (ret)
		throw std::string("fi_enable() failed");

	postRecv();

	{
		//TODO Just to resolve the addr
		Fabric info(mFabric.attr(), node, serv, false);
		ret = fi_connect(mEp, info.info()->dest_addr , NULL, 0);
		if (ret)
			throw std::string("fi_connect() failed");
	}
}

void FabricConnection::send(const std::string &msg)
{
	if (msg.length() > mBuffSize)
		throw std::string("invalid message size");

	ssize_t sret;

	uint64_t *len = (uint64_t *)mTxBuff;
	*len = msg.length();

	uint8_t *data = (uint8_t *)(len + 1);
	memcpy(data, msg.data(), msg.length());

	sret = fi_send(mEp, mTxBuff, sizeof(uint64_t) + msg.length(),
			fi_mr_desc(mTxMr), 0, NULL);
	if (sret < 0)
		throw std::string("fi_send() failed");

	struct fi_cq_msg_entry entry;
	sret = fi_cq_sread(mTxCq, &entry, 1, NULL, -1);
	if (sret < 0)
		throw std::string("fi_cq_sread() failed");

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
