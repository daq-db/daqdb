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

#include <cstddef>
#include <cstring>

#include "Daqdb.h"

using namespace std;

namespace DaqDB {

size_t getDaqdbDhtMsgSize(size_t keySize, size_t valSize) {
    return sizeof(DaqdbDhtMsg) + valSize + keySize;
}

const char *getValueFromDaqdbDhtMsg(const DaqdbDhtMsg *msg) {
    return (msg->msg + msg->keySize);
}

void fillDaqdbMsg(DaqdbDhtMsg *msg, int opcode, const char *key, size_t keySize,
                  const char *val, size_t valSize) {
    msg->signature = DAQDB_MSG_SIGNATURE;
    msg->opcode = opcode;
    msg->keySize = keySize;
    msg->valSize = valSize;
    memcpy(msg->msg, key, keySize);
    memcpy(msg->msg + keySize, val, valSize);
}

bool fillRqstResponse(char **result, size_t *resultSize,
                      DaqdbDhtResult *response) {
    if (checkDaqdbSignature(response->signature)) {
        *result = (char *)calloc(1, response->msgSize);
        memcpy(*result, response->msg, response->msgSize);
        *resultSize = response->msgSize;
        return true;
    } else {
        return false;
    }
}

size_t fillDaqdbResult(DaqdbDhtResult *result, int rc, const char *msg,
                       size_t msgSize) {
    result->signature = DAQDB_MSG_SIGNATURE;
    result->rc = rc;
    result->msgSize = msgSize;
    if (msgSize > 0) {
        memcpy(result->msg, msg, msgSize);
    }
    return sizeof(DaqdbDhtResult) + msgSize;
}

bool checkDaqdbSignature(uint64_t signature) {
    return (signature == DAQDB_MSG_SIGNATURE);
}

bool checkDaqdbSignature(const char *sendbuf, const size_t sendcount) {
    if (sendcount < sizeof(DaqdbDhtMsg)) {
        return false;
    }
    DaqdbDhtMsg *msg = (DaqdbDhtMsg *)sendbuf;
    return checkDaqdbSignature(msg->signature);
}

} // namespace DaqDB
