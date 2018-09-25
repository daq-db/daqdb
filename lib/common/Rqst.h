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

struct ValCtx {
    uint8_t location = 0;
    size_t size = 0;
    void *val = nullptr;
};

template <class T>
class Rqst {
  public:
    Rqst(const T op, const char *key, const size_t keySize,
         const char *value, size_t valueSize,
         KVStoreBase::KVStoreBaseCallback clb)
        : op(op), key(key), keySize(keySize), value(value),
          valueSize(valueSize), clb(clb) {}

    const T op;
    const char *key = nullptr;
    size_t keySize = 0;
    const char *value = nullptr;
    size_t valueSize = 0;

    // @TODO jradtke need to check if passing function object has impact on
    // performance
    KVStoreBase::KVStoreBaseCallback clb;
};

} // namespace DaqDB
