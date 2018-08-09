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

#ifndef FABRIC_MEM_REGION_H
#define FABRIC_MEM_REGION_H

#include <stdint.h>
#include <rdma/fi_domain.h>

#include <fabric/Fabric.h>

namespace Fabric
{

enum FabricMRPerm {
	SEND = FI_SEND,
	RECV = FI_RECV,
	READ = FI_READ,
	WRITE = FI_WRITE,
	REMOTE_READ = FI_REMOTE_READ,
	REMOTE_WRITE = FI_REMOTE_WRITE,
};

class FabricMR {
public:
	FabricMR(Fabric &fabric, void *ptr, size_t size, uint64_t perm);
	virtual ~FabricMR();

	void *getLKey();
	uint64_t getRKey();
	void *getPtr();
	size_t getSize();
protected:

	struct fid_mr *mMr;
	void *mPtr;
	size_t mSize;
};

}

#endif // FABRIC_MEM_REGION_H
