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
#ifndef FABRIC_MEM_REGION_H
#define FABRIC_MEM_REGION_H

#include <stdint.h>
#include <rdma/fi_domain.h>
#include "Fabric.h"

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
