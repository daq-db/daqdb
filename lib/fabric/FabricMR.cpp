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

#include <fabric/FabricMR.h>

namespace Fabric
{


FabricMR::FabricMR(Fabric &fabric, void *ptr, size_t size, uint64_t perm)
	: mPtr(ptr), mSize(size)
{
	int ret = fi_mr_reg(fabric.domain(), mPtr, mSize, perm, 0, 0, 0, &mMr, NULL);
	if (ret)
		throw std::string("fi_mr_reg failed");

}

FabricMR::~FabricMR()
{
	fi_close(&mMr->fid);
}

void *FabricMR::getLKey()
{
	return fi_mr_desc(mMr);
}

uint64_t FabricMR::getRKey()
{
	return fi_mr_key(mMr);
}

void *FabricMR::getPtr()
{
	return mPtr;
}

size_t FabricMR::getSize()
{
	return mSize;
}

}
