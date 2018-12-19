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

#include <Logger.h>
#include <PrimaryKeyNextQueue.h>
#include <iostream>

namespace DaqDB {

PrimaryKeyNextQueue::PrimaryKeyNextQueue(const DaqDB::Options &options)
    : PrimaryKeyBase(options) {
    std::cout << "Initializing NextQueue for primary keys of size "
              << std::to_string(options.runtime.maxReadyKeys) << std::endl;
    _readyKeys =
        spdk_ring_create(SPDK_RING_TYPE_MP_MC, options.runtime.maxReadyKeys,
                         SPDK_ENV_SOCKET_ID_ANY);
    if (!_readyKeys) {
        DAQ_DEBUG("Cannnot create SPDK ring for ready keys");
        throw OperationFailedException(ALLOCATION_ERROR);
    }
}

PrimaryKeyNextQueue::~PrimaryKeyNextQueue() {}

char *PrimaryKeyNextQueue::_createPKeyBuff(char *srcKeyBuff) {
    char *keyBuff = new char[_keySize];
    std::memset(keyBuff, 0, _keySize);
    std::memcpy(keyBuff + _pKeyOffset, srcKeyBuff + _pKeyOffset, _pKeySize);
    return keyBuff;
}

Key PrimaryKeyNextQueue::dequeueNext() {
    char *pKeyBuff;
    int cnt =
        spdk_ring_dequeue(_readyKeys, reinterpret_cast<void **>(&pKeyBuff), 1);
    if (!cnt)
        throw OperationFailedException(Status(KEY_NOT_FOUND));
    return Key(pKeyBuff, _keySize);
}

void PrimaryKeyNextQueue::enqueueNext(Key &&key) {
    if (!isLocal(key))
        return;
    char *pKeyBuff = _createPKeyBuff(key.data());
    int cnt =
        spdk_ring_enqueue(_readyKeys, reinterpret_cast<void **>(&pKeyBuff), 1);
    if (!cnt) {
        delete[] pKeyBuff;
        throw OperationFailedException(QUEUE_FULL_ERROR);
    }
}

} // namespace DaqDB
