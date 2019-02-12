/**
 *  Copyright (c) 2019 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License. 
 */

#pragma once

#include <daqdb/Status.h>

namespace DaqDB {

typedef void (*DhtContFunc)(void *appCtx, void *ioCtx);
typedef std::map<std::pair<int, int>, DhtNode *> RangeToHost;

enum class ErpRequestType : std::uint8_t {
    ERP_REQUEST_GET = 2,
    ERP_REQUEST_GETANY,
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
