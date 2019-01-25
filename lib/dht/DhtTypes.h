/**
 * Copyright 2019 Intel Corporation.
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

#pragma once

#include <daqdb/Status.h>

// @TODO : jradtke Cannot include rpc.h, net / if.h conflict
namespace erpc {
class Nexus;
class MsgBuffer;
} // namespace erpc

namespace DaqDB {

typedef void (*DhtContFunc)(void *appCtx, void *ioCtx);
typedef std::map<std::pair<int, int>, DhtNode *> RangeToHost;

enum class ErpRequestType : std::uint8_t {
    ERP_REQUEST_GET = 2,
    ERP_REQUEST_PUT,
    ERP_REQUEST_REMOVE
};

struct DaqdbDhtMsg {
    size_t keySize;
    size_t valSize;
    // Contains both key and value
    char msg[];
};

struct DaqdbDhtResult {
    StatusCode status;
    size_t msgSize;
    // Contains value for get requests
    char msg[];
};
} // namespace DaqDB
