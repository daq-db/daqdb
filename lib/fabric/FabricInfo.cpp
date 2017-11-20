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

#include "FabricInfo.h"

#include <sstream>
#include <string.h>

#define FIVERSION FI_VERSION(1, 4)

namespace Fabric {

FabricInfo::FabricInfo() :
	mInfo(nullptr),
	mHints(nullptr)
{
}

FabricInfo::FabricInfo(struct fi_info *info) :
	mInfo(info),
	mHints(nullptr)
{
}

FabricInfo::FabricInfo(const FabricAttributes &attr, const std::string &node, const std::string &serv, bool listener)
{
	mHints = fi_allocinfo();
	if (mHints == nullptr)
		throw std::string("cannot allocate fi_info");

	mHints->ep_attr->type = FI_EP_MSG;
	mHints->domain_attr->mr_mode = FI_MR_BASIC;
	mHints->domain_attr->threading = FI_THREAD_SAFE;
	mHints->caps = FI_MSG | FI_RMA;
	mHints->mode = FI_CONTEXT | FI_LOCAL_MR | FI_RX_CQ_DATA;
	mHints->tx_attr->msg_order = FI_ORDER_RAW | FI_ORDER_SAW;
	mHints->addr_format = FI_SOCKADDR;

	if (attr.mProv != "") {
		mHints->fabric_attr->prov_name = strdup(attr.mProv.c_str());
		if (mHints->fabric_attr->prov_name == nullptr)
			throw std::string("cannot allocate buffer "
					"for provider name");
	}

	int ret;

	uint64_t flags = 0;

	if (listener)
		flags |= FI_SOURCE;

	ret = fi_getinfo(FIVERSION, node.c_str(), serv.c_str(),
			flags, mHints, &mInfo);
	if (ret)
		throw std::string("cannot get fabric interface information");

}

FabricInfo::~FabricInfo()
{
	if (mHints)
		fi_freeinfo(mHints);
	fi_freeinfo(mInfo);
}

std::string FabricInfo::addr2str(const struct sockaddr_in &addr)
{
	std::stringstream ss;
	ss << inet_ntoa(addr.sin_addr);
	ss << ":";
	ss << ntohs(addr.sin_port);

	return ss.str();
}

std::string FabricInfo::getPeerStr()
{
	if (mInfo->dest_addr == nullptr)
		return "";

	if (mInfo->addr_format == FI_SOCKADDR_IN)
		return addr2str(*(struct sockaddr_in *)mInfo->dest_addr);

	return "unknown";
}

std::string FabricInfo::getNameStr()
{
	if (mInfo->src_addr == nullptr)
		return "";

	if (mInfo->addr_format == FI_SOCKADDR_IN)
		return addr2str(*(struct sockaddr_in *)mInfo->src_addr);

	return "unknown";
}

struct fi_info *FabricInfo::info()
{
	return mInfo;
}

}
