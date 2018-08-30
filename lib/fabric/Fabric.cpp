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

#include <fabric/Fabric.h>
#include <string.h>
#include "common.h"

namespace Fabric {

Fabric::Fabric(const FabricAttributes &attr, const std::string &node,
		const std::string &serv, bool listener) :
	mInfo(attr, node, serv, listener),
	mHints(NULL),
	mFabric(NULL),
	mDomain(NULL),
	mEq(NULL)
{
	int ret;

	memset(&mEqAttr, 0, sizeof(mEqAttr));

	ret = fi_fabric(mInfo.info()->fabric_attr, &mFabric, NULL);
	if (ret)
		FATAL("fi_fabric() failed");

	ret = fi_domain(mFabric, mInfo.info(), &mDomain, NULL);
	if (ret)
		FATAL("fi_domain() failed");

	mEqAttr.size = 0;
	mEqAttr.flags = FI_WRITE;
	mEqAttr.wait_obj = FI_WAIT_UNSPEC;
	mEqAttr.signaling_vector = 0;
	mEqAttr.wait_set = NULL;

	ret = fi_eq_open(mFabric, &mEqAttr, &mEq, NULL);
	if (ret)
		FATAL("fi_eq_open() failed");
}

Fabric::~Fabric()
{
	fi_close(&mEq->fid);
	fi_close(&mDomain->fid);
	fi_close(&mFabric->fid);
}

struct fi_info *Fabric::info()
{
	return mInfo.info();
}

struct fid_fabric *Fabric::fabric()
{
	return mFabric;
}

struct fid_domain *Fabric::domain()
{
	return mDomain;
}

struct fid_eq *Fabric::eq()
{
	return mEq;
}

}
