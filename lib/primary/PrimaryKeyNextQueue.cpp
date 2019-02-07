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

#include <Logger.h>
#include <PrimaryKeyNextQueue.h>

namespace DaqDB {

PrimaryKeyNextQueue::PrimaryKeyNextQueue(const DaqDB::Options &options)
    : PrimaryKeyBase(options) {
    DAQ_INFO("Initializing NextQueue for primary keys of size " +
             std::to_string(options.runtime.maxReadyKeys));
    _readyKeys =
        spdk_ring_create(SPDK_RING_TYPE_MP_MC, options.runtime.maxReadyKeys,
                         SPDK_ENV_SOCKET_ID_ANY);
    if (!_readyKeys) {
        DAQ_CRITICAL("Cannnot create SPDK ring for ready keys");
        throw OperationFailedException(ALLOCATION_ERROR);
    }
}

PrimaryKeyNextQueue::~PrimaryKeyNextQueue() {
    char *pKeyBuff;
    while (
        spdk_ring_dequeue(_readyKeys, reinterpret_cast<void **>(&pKeyBuff), 1))
        delete[] pKeyBuff;
    spdk_ring_free(_readyKeys);
}

char *PrimaryKeyNextQueue::_createPKeyBuff(const char *srcKeyBuff) {
    char *pKeyBuff = new char[_pKeySize];
    std::memcpy(pKeyBuff, srcKeyBuff + _pKeyOffset, _pKeySize);
    return pKeyBuff;
}

void PrimaryKeyNextQueue::dequeueNext(Key &key) {
    char *pKeyBuff;
    int cnt =
        spdk_ring_dequeue(_readyKeys, reinterpret_cast<void **>(&pKeyBuff), 1);
    if (!cnt)
        throw OperationFailedException(Status(KEY_NOT_FOUND));
    std::memset(key.data(), 0, _keySize);
    std::memcpy(key.data() + _pKeyOffset, pKeyBuff, _pKeySize);
    delete[] pKeyBuff;
}

void PrimaryKeyNextQueue::enqueueNext(const Key &key) {
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
