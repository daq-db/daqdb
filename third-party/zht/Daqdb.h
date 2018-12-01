/**
 * Copyright 2018 Intel Corporation.
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

#include <cstddef>
#include <cstdint>
#include <map>
#include <string>

#include <daqdb/KVStoreBase.h>

typedef std::map<std::pair<int, int>, int> RangeToHost;

namespace DaqDB {

const uint64_t DAQDB_MSG_SIGNATURE = 0x11223344;
const int OPC_INSERT = 3;
const int OPC_GET = 1;

struct DaqdbDhtMsg {
    uint64_t signature;
    int opcode;
    size_t keySize;
    size_t valSize;
    // Contains both key and value
    char msg[];
};

struct DaqdbDhtResult {
    uint64_t signature;
    int rc;
    size_t msgSize;
    // Contains value for get requests
    char msg[];
};

size_t getDaqdbDhtMsgSize(size_t keySize, size_t valSize);

const char *getValueFromDaqdbDhtMsg(const DaqdbDhtMsg *msg);

void fillDaqdbMsg(DaqdbDhtMsg *msg, int opcode, const char *key, size_t keySize,
                  const char *val, size_t valSize);

bool fillRqstResponse(char **result, size_t *resultSize,
                      DaqdbDhtResult *response);

size_t fillDaqdbResult(DaqdbDhtResult *result, int rc, const char *msg,
                       size_t msgSize);

bool checkDaqdbSignature(uint64_t signature);

bool checkDaqdbSignature(const char *sendbuf, const size_t sendcount);

} // namespace DaqDB
