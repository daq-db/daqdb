/**
 * Copyright 2018-2019 Intel Corporation.
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
