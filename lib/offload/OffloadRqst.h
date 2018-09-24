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

#pragma once

#include <cstddef>
#include <cstdint>

#include <daqdb/KVStoreBase.h>

namespace DaqDB {

enum class OffloadOperation : std::int8_t { NONE = 0, GET, UPDATE, REMOVE };

class OffloadRqst {
  public:
    OffloadRqst(const OffloadOperation op, const char *key,
                const size_t keySize, const char *value, size_t valueSize,
                KVStoreBase::KVStoreBaseCallback clb)
        : op(op), key(key), keySize(keySize), value(value),
          valueSize(valueSize), clb(clb) {}

    const OffloadOperation op = OffloadOperation::NONE;
    const char *key = nullptr;
    size_t keySize = 0;
    const char *value = nullptr;
    size_t valueSize = 0;

    // @TODO jradtke need to check if passing function object has impact on performance
    KVStoreBase::KVStoreBaseCallback clb;
};

}
