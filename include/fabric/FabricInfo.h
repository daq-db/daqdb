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

#ifndef FABRIC_INFO_HPP
#define FABRIC_INFO_HPP

#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <rdma/fabric.h>
#include <rdma/fi_domain.h>

#include <fabric/FabricAttributes.h>

namespace Fabric {

class FabricInfo {
public:
	static std::string addr2str(const struct sockaddr_in &addr);

public:
	FabricInfo();
	explicit FabricInfo(struct fi_info *info);
	FabricInfo(const FabricAttributes &attr, const std::string &node, const std::string &serv, bool listener);
	virtual ~FabricInfo();

	std::string getPeerStr();
	std::string getNameStr();

	struct fi_info *info();
protected:
	struct fi_info *mInfo;
	struct fi_info *mHints;
};

}
#endif // FABRIC_INFO_HPP
