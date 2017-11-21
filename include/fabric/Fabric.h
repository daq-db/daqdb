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
#ifndef FABRIC_HPP
#define FABRIC_HPP

#include <rdma/fabric.h>
#include <rdma/fi_domain.h>
#include "../../include/fabric/FabricAttributes.h"
#include "../../include/fabric/FabricInfo.h"

namespace Fabric {

class Fabric {
public:
	Fabric(const FabricAttributes &attr, const std::string &node,
		const std::string &serv, bool listener);
	virtual ~Fabric();

	FabricAttributes attr() { return mAttr; }

	struct fi_info *info();
	struct fid_fabric *fabric();
	struct fid_domain *domain();
	struct fid_eq *eq();
protected:
	FabricAttributes mAttr;
	FabricInfo mInfo;
	struct fi_info *mHints;
	struct fid_fabric *mFabric;
	struct fid_domain *mDomain;
	struct fi_eq_attr mEqAttr;
	struct fid_eq *mEq;
};

}

#endif // FABRIC_HPP
