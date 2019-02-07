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

    _localKeyValue = options.dht.id;
    _localKeyMask = (1U << options.dht.neighbors.size()) - 1;
    DAQ_INFO("  Local key value: " + std::to_string(_localKeyValue));
    DAQ_INFO("  Local key mask: " + std::to_string(_localKeyMask));
    if (_pKeySize > sizeof(uint64_t))
        throw OperationFailedException(NOT_SUPPORTED);
}

PrimaryKeyBase::~PrimaryKeyBase() {}

void PrimaryKeyBase::dequeueNext(Key &key) { throw FUNC_NOT_SUPPORTED; }

void PrimaryKeyBase::enqueueNext(const Key &Key) {
    // don't throw exception, just do nothing
}

bool PrimaryKeyBase::isLocal(const Key &key) {
    const uint64_t *pKeyPtr =
        reinterpret_cast<const uint64_t *>(key.data() + _pKeyOffset);
    return ((*pKeyPtr & _localKeyMask) == _localKeyValue);
}

} // namespace DaqDB
