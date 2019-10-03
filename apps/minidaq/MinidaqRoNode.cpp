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

#include "MinidaqRoNode.h"
#include <libpmem.h>
#include <iostream>
#include <random>

namespace DaqDB {

thread_local uint64_t MinidaqRoNode::_eventId;
thread_local constexpr char MinidaqRoNode::_data_buffer[100000];
thread_local std::mt19937 _roGenerator;

MinidaqRoNode::MinidaqRoNode(KVStoreBase *kvs) : MinidaqNode(kvs) {
    SetFragmentDistro("const");
}

MinidaqRoNode::~MinidaqRoNode() {}

std::string MinidaqRoNode::_GetType() { return std::string("readout"); }

void MinidaqRoNode::_Setup(int executorId) { _eventId = executorId; }

void MinidaqRoNode::SetFragmentDistro(const std::string &distro) {
    if (!distro.compare("const")) {
        _nextFragmentSize = [this]() { return _fSize; };
    } else if (!distro.compare("poisson")) {
        std::cout << "### Using Poisson distribution" << std::endl;
        _nextFragmentSize = [this]() {
            thread_local std::poisson_distribution<size_t> distro(_fSize);
            size_t s;

            while (!(s = distro(_roGenerator)))
                ;

            return s;
        };
    } else {
        std::cout << "### Unsupported distribution" << std::endl;
        throw OperationFailedException(Status(NOT_IMPLEMENTED));
    }
}

Key MinidaqRoNode::_NextKey() {
    Key key = _kvs->AllocKey(_localOnly
                                 ? AllocOptions(KeyValAttribute::NOT_BUFFERED)
                                 : AllocOptions(KeyValAttribute::KVS_BUFFERED));
    MinidaqKey *mKeyPtr = reinterpret_cast<MinidaqKey *>(key.data());
    mKeyPtr->detectorId = _id;
    mKeyPtr->componentId = 0;
    memcpy(&mKeyPtr->eventId, &_eventId, sizeof(mKeyPtr->eventId));
    _eventId += _nTh;
    return key;
}

unsigned int pppp = 0;
const unsigned int p_o_quant = 100000;

void MinidaqRoNode::_Task(Key &&key, std::atomic<std::uint64_t> &cnt,
                          std::atomic<std::uint64_t> &cntErr) {
    DaqDB::Value value;
    try {
        value = _kvs->Alloc(key, _nextFragmentSize());
    }
    catch (...) {
        _kvs->Free(std::move(key));
        throw;
    }

    pmem_memcpy_nodrain(value.data(), _data_buffer, value.size());

#ifdef WITH_INTEGRITY_CHECK
    _FillBuffer(key, value.data(), value.size());
    _CheckBuffer(key, value.data(), value.size());
#endif /* WITH_INTEGRITY_CHECK */

    if ( !((pppp++)%p_o_quant) ) {
            std::cout << "PPPP " << pppp << std::endl;
    }
    _kvs->Put(std::move(key), std::move(value));
    cnt++;
}

void MinidaqRoNode::SetFragmentSize(size_t s) { _fSize = s; }

void MinidaqRoNode::SetSubdetectorId(int id) { _id = id; }
}
