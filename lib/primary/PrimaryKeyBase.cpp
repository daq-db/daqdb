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

#include <PrimaryKeyBase.h>
#include <iostream>

namespace DaqDB {

PrimaryKeyBase::PrimaryKeyBase(const DaqDB::Options &options)
    : _keySize(0), _pKeySize(0), _pKeyOffset(0) {
    std::cout << "Initializing Primary Key Base Engine" << std::endl;

    for (size_t i = 0; i < options.key.nfields(); i++) {
        if (options.key.field(i).isPrimary) {
            _pKeySize = options.key.field(i).size;
            _pKeyOffset = _keySize;
        }
        _keySize += options.key.field(i).size;
    }

    std::cout << "  Total key size: " << std::to_string(_keySize) << std::endl
              << "  Primary key size: " << std::to_string(_pKeySize)
              << std::endl
              << "  Primary key offset: " << std::to_string(_pKeyOffset)
              << std::endl;

    _localKeyValue = options.dht.id;
    _localKeyMask = (1U << options.dht.neighbors.size()) - 1;
    std::cout << "  Local key value: " << std::to_string(_localKeyValue)
              << std::endl
              << "  Local key mask: " << std::to_string(_localKeyMask)
              << std::endl;
    if (_pKeySize > sizeof(uint64_t))
        throw OperationFailedException(NOT_SUPPORTED);
}

PrimaryKeyBase::~PrimaryKeyBase() {}

Key PrimaryKeyBase::dequeueNext() {
    throw OperationFailedException(NOT_SUPPORTED);
}

void PrimaryKeyBase::enqueueNext(Key &&Key) {}

bool PrimaryKeyBase::isLocal(const Key &key) {
    const char *keyBuff = reinterpret_cast<const char *>(key.data());
    const uint64_t *pKeyPtr =
        reinterpret_cast<const uint64_t *>(keyBuff + _pKeyOffset);
    return ((*pKeyPtr & _localKeyMask) == _localKeyValue);
}

} // namespace DaqDB
