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

#include "MinidaqFfNode.h"
#include <random>

namespace DaqDB {

/** @todo conisder moving to a new MinidaqSelector class */
thread_local std::mt19937 _generator;

MinidaqFfNode::MinidaqFfNode(KVStoreBase *kvs) : MinidaqNode(kvs) {}

MinidaqFfNode::~MinidaqFfNode() {}

std::string MinidaqFfNode::_GetType() { return std::string("fast-filtering"); }

void MinidaqFfNode::_Setup(int executorId) {}

Key MinidaqFfNode::_NextKey() {
    for (int i = 0; i < _maxRetries; i++) {
        try {
            return _kvs->GetAny(
                _localOnly ? AllocOptions(KeyValAttribute::NOT_BUFFERED)
                           : AllocOptions(KeyValAttribute::KVS_BUFFERED),
                GetOptions(READY));
        } catch (OperationFailedException &e) {
            if (e.status()() == KEY_NOT_FOUND) {
                /* Wait until new key is availabile. */
                if (_delay_us) {
                    std::this_thread::sleep_for(
                        std::chrono::microseconds(_delay_us));
                }
                continue;
            } else {
                throw;
            }
        }
    }
    throw OperationFailedException(Status(KEY_NOT_FOUND));
}

bool MinidaqFfNode::_Accept() {
    bool accept = false;
    if (_acceptLevel >= 1.0) {
        accept = true;
    } else if (_acceptLevel > 0.0) {
        static thread_local std::bernoulli_distribution distro(_acceptLevel);
        accept = distro(_generator);
    }
    return accept;
}

int MinidaqFfNode::_PickSubdetector() {
    /** @todo add distro */
    return _baseId;
}

int MinidaqFfNode::_PickNFragments() {
    /** @todo add distro */
    return _nSubdetectors;
}

void MinidaqFfNode::_Task(Key &&key, std::atomic<std::uint64_t> &cnt,
                          std::atomic<std::uint64_t> &cntErr) {
    MinidaqKey *mKeyPtr = reinterpret_cast<MinidaqKey *>(key.data());
    int baseId = _PickSubdetector();
    bool accept = _Accept();

    for (int i = 0; i < _PickNFragments(); i++) {
        /** @todo change to GetRange once implemented */
        mKeyPtr->componentId = baseId + i;
        DaqDB::Value value;
        try {
            value = _kvs->Get(key);
        } catch (...) {
            _kvs->Free(std::move(key));
            throw;
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
                    _kvs->Free(std::move(key));
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
            _kvs->Free(std::move(key));
            cnt++;
        }
    }
}

void MinidaqFfNode::SetBaseSubdetectorId(int id) { _baseId = id; }

void MinidaqFfNode::SetSubdetectors(int n) { _nSubdetectors = n; }

void MinidaqFfNode::SetAcceptLevel(double p) { _acceptLevel = p; }

void MinidaqFfNode::SetRetryDelay(int d) { _delay_us = d; }
}
