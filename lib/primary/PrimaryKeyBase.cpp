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
#include <PrimaryKeyBase.h>

namespace DaqDB {

static uint64_t modulo(const uint8_t* arr, int size, uint64_t div) {
    uint64_t val = 0;
    while (--size >= 0)
        val |= arr[size] << size;
    return val % div;
}

PrimaryKeyBase::PrimaryKeyBase(const DaqDB::Options &options)
    : _keySize(0), _pKeySize(0), _pKeyOffset(0) {
    DAQ_INFO("Initializing Primary Key Base Engine");

    for (size_t i = 0; i < options.key.nfields(); i++) {
        if (options.key.field(i).isPrimary) {
            _pKeySize = options.key.field(i).size;
            _pKeyOffset = _keySize;
        }
        _keySize += options.key.field(i).size;
    }

    DAQ_INFO("  Total key size: " + std::to_string(_keySize));
    DAQ_INFO("  Primary key size: " + std::to_string(_pKeySize));
    DAQ_INFO("  Primary key offset: " + std::to_string(_pKeyOffset));

    _localNodeId  = options.dht.id;
    _nNodes = options.dht.neighbors.size();
    DAQ_INFO("  Local node ID: " + std::to_string(_localNodeId));
    DAQ_INFO("  Total number of nodes: " + std::to_string(_nNodes));
    if (_pKeySize > sizeof(uint64_t))
        throw OperationFailedException(NOT_SUPPORTED);
}

PrimaryKeyBase::~PrimaryKeyBase() {}

void PrimaryKeyBase::dequeueNext(Key &key) { throw FUNC_NOT_SUPPORTED; }

void PrimaryKeyBase::enqueueNext(const Key &Key) {
    // don't throw exception, just do nothing
}

bool PrimaryKeyBase::isLocal(const Key &key) {
    bool local = false;
    const uint8_t *pKeyPtr =
        reinterpret_cast<const uint8_t *>(key.data() + _pKeyOffset);
    local = modulo(pKeyPtr, _pKeySize, _nNodes) == _localNodeId;
    DAQ_DEBUG("Primary key is local: " + std::to_string(local));
    return local;
}

} // namespace DaqDB
