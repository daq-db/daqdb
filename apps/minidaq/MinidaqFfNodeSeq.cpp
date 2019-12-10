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

#include "MinidaqFfNodeSeq.h"

namespace DaqDB {

thread_local uint64_t MinidaqFfNodeSeq::_eventId;

MinidaqFfNodeSeq::MinidaqFfNodeSeq(KVStoreBase *kvs) : MinidaqFfNode(kvs) {}

MinidaqFfNodeSeq::~MinidaqFfNodeSeq() {}

std::string MinidaqFfNodeSeq::_GetType() {
    return std::string("fast-filtering-seq");
}

void MinidaqFfNodeSeq::_Setup(int executorId) { _eventId = executorId; }

Key MinidaqFfNodeSeq::_NextKey() {
    Key key = _kvs->AllocKey(_localOnly
                                 ? AllocOptions(KeyValAttribute::NOT_BUFFERED)
                                 : AllocOptions(KeyValAttribute::KVS_BUFFERED));
    MinidaqKey *mKeyPtr = reinterpret_cast<MinidaqKey *>(key.data());
    mKeyPtr->detectorId = _baseId;
    mKeyPtr->componentId = 0;
    memcpy(&mKeyPtr->eventId, &_eventId, sizeof(mKeyPtr->eventId));
    _eventId += _nTh;
    return key;
}

void MinidaqFfNodeSeq::_Task(Key &&key, std::atomic<std::uint64_t> &cnt,
                             std::atomic<std::uint64_t> &cntErr) {
    MinidaqKey *mKeyPtr = reinterpret_cast<MinidaqKey *>(key.data());
    int baseId = _PickSubdetector();
    bool accept = _Accept();
    int nRetries;

    for (int i = 0; i < _PickNFragments(); i++) {
        /** @todo change to GetRange once implemented */
        mKeyPtr->detectorId = baseId + i;
        DaqDB::Value value;
        nRetries = 0;
        while (nRetries < _maxRetries) {
            nRetries++;
            try {
                value = _kvs->Get(key);
            } catch (OperationFailedException &e) {
                if ((e.status()() == KEY_NOT_FOUND) &&
                    (nRetries < _maxRetries)) {
                    /* Wait until it is availabile. */
                    if (_delay_us) {
                        std::this_thread::sleep_for(
                            std::chrono::microseconds(_delay_us));
                    }
                    continue;
                } else {
                    _kvs->Free(std::move(key));
                    throw;
                }
            } catch (...) {
                _kvs->Free(std::move(key));
                throw;
            }
            break;
        }
#ifdef WITH_INTEGRITY_CHECK
        if (!_CheckBuffer(key, value.data(), value.size())) {
            throw OperationFailedException(Status(UNKNOWN_ERROR));
        }
#endif /* WITH_INTEGRITY_CHECK */
        if (accept) {
            while (1) {
                try {
                    _kvs->UpdateAsync(
                        key, UpdateOptions(LONG_TERM),
                        [&cnt, &cntErr](DaqDB::KVStoreBase *kvs,
                                        DaqDB::Status status, const char *key,
                                        const size_t keySize, const char *value,
                                        const size_t valueSize) {
                            if (!status.ok()) {
                                cntErr++;
                            } else {
                                cnt++;
                            }
                        });
                    /** @todo c++ does not allow it in lambda,
                     *        this is not thread-safe
                     */
                    _kvs->Free(key, std::move(value));
                } catch (QueueFullException &e) {
                    // Keep retrying
                    if (_delay_us) {
                        std::this_thread::sleep_for(
                            std::chrono::microseconds(_delay_us));
                    }
                    continue;
                } catch (...) {
                    _kvs->Free(key, std::move(value));
                    _kvs->Free(std::move(key));
                    throw;
                }
                break;
            }
        } else {
            _kvs->Remove(key);
            _kvs->Free(key, std::move(value));
            cnt++;
        }
    }
    _kvs->Free(std::move(key));
}
}
