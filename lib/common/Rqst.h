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

#include "ClassAlloc.h"
#include "GeneralPool.h"
#include <daqdb/KVStoreBase.h>

namespace DaqDB {

struct ValCtx {
    uint8_t location = 0;
    size_t size = 0;
    void *val = nullptr;
};

struct ConstValCtx {
    uint8_t location = 0;
    size_t size = 0;
    const void *val = nullptr;
};

template <class T>
class Rqst {
  public:
    Rqst(const T op, const char *key, const size_t keySize, const char *value,
         size_t valueSize, KVStoreBase::KVStoreBaseCallback clb,
         uint8_t loc = 0)
        : op(op), key(key), keySize(keySize), value(value),
          valueSize(valueSize), clb(clb), loc(loc) {
        memcpy(keyBuffer, key, keySize);
    }
    Rqst()
        : op(T::GET), key(keyBuffer), keySize(0), value(0), valueSize(0),
          clb(0), loc(0){};
    virtual ~Rqst() = default;
    void finalizeUpdate(const char *_key, const size_t _keySize,
                        const char *_value, size_t _valueSize,
                        KVStoreBase::KVStoreBaseCallback _clb,
                        uint8_t _loc = 0) {
        op = T::UPDATE;
        key = _key;
        keySize = _keySize;
        memcpy(keyBuffer, key, keySize);
        key = keyBuffer;
        value = _value;
        valueSize = _valueSize;
        clb = _clb;
        loc = _loc;
    }
    void finalizeGet(const char *_key, const size_t _keySize,
                     const char *_value, size_t _valueSize,
                     KVStoreBase::KVStoreBaseCallback _clb) {
        op = T::GET;
        key = _key;
        keySize = _keySize;
        value = _value;
        valueSize = _valueSize;
        clb = _clb;
    }
    void finalizeRemove(const char *_key, const size_t _keySize,
                        const char *_value, size_t _valueSize,
                        KVStoreBase::KVStoreBaseCallback _clb) {
        op = T::REMOVE;
        key = _key;
        keySize = _keySize;
        value = _value;
        valueSize = _valueSize;
        clb = _clb;
    }

    T op;
    const char *key = nullptr;
    char keyBuffer[32];
    size_t keySize = 0;
    const char *value = nullptr;
    size_t valueSize = 0;

    // @TODO jradtke need to check if passing function object has impact on
    // performance
    KVStoreBase::KVStoreBaseCallback clb;
    uint8_t loc;
    unsigned char taskBuffer[256];

    static MemMgmt::GeneralPool<Rqst, MemMgmt::ClassAlloc<Rqst>> updatePool;
    static MemMgmt::GeneralPool<Rqst, MemMgmt::ClassAlloc<Rqst>> getPool;
    static MemMgmt::GeneralPool<Rqst, MemMgmt::ClassAlloc<Rqst>> removePool;
};

template <class T>
MemMgmt::GeneralPool<Rqst<T>, MemMgmt::ClassAlloc<Rqst<T>>>
    Rqst<T>::updatePool(100, "updateRqstPool");

template <class T>
MemMgmt::GeneralPool<Rqst<T>, MemMgmt::ClassAlloc<Rqst<T>>>
    Rqst<T>::getPool(100, "getRqstPool");

template <class T>
MemMgmt::GeneralPool<Rqst<T>, MemMgmt::ClassAlloc<Rqst<T>>>
    Rqst<T>::removePool(100, "removeRqstPool");

} // namespace DaqDB
