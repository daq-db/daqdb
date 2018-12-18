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

#include "MinidaqFfNodeSeq.h"

namespace DaqDB {

thread_local int _eventId;

MinidaqFfNodeSeq::MinidaqFfNodeSeq(KVStoreBase *kvs) : MinidaqFfNode(kvs) {}

MinidaqFfNodeSeq::~MinidaqFfNodeSeq() {}

std::string MinidaqFfNodeSeq::_GetType() {
    return std::string("fast-filtering-seq");
}

void MinidaqFfNodeSeq::_Setup(int executorId) { _eventId = executorId; }

Key MinidaqFfNodeSeq::_NextKey() {
    Key key = _kvs->AllocKey();
    MinidaqKey *mKeyPtr = reinterpret_cast<MinidaqKey *>(key.data());
    mKeyPtr->runId = _runId;
    mKeyPtr->subdetectorId = _baseId;
    mKeyPtr->eventId = _eventId;
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
        mKeyPtr->subdetectorId = baseId + i;
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
                    _kvs->Free(std::move(key));
                } catch (QueueFullException &e) {
                    // Keep retrying
                    if (_delay_us) {
                        std::this_thread::sleep_for(
                            std::chrono::microseconds(_delay_us));
                    }
                    continue;
                } catch (...) {
                    _kvs->Free(std::move(key));
                    delete value.data();
                    throw;
                }
                break;
            }
        } else {
            _kvs->Remove(key);
            _kvs->Free(std::move(key));
            cnt++;
        }
        delete value.data();
    }
}
}
