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
    Rqst() {};

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
