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

#ifndef FABRIC_HPP
#define FABRIC_HPP

#include <rdma/fabric.h>
#include <rdma/fi_domain.h>

#include <fabric/FabricAttributes.h>
#include <fabric/FabricInfo.h>

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
