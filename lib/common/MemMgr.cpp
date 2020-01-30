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

#include <iostream>

#include "MemMgr.h"

#ifdef _MM_GMP_ON_
DaqDB::MemMgr theMemMgr;
#endif

namespace DaqDB {
bool MemMgr::started = false;

MemMgr::MemMgr(const AllocStrategy &strategy_)
    : GMPpool4(10001, "GMPpool4", strategy_),
      GMPpool8(10002, "GMPpool8", strategy_),
      GMPpool16(10003, "GMPpool16", strategy_),
      GMPpool32(10004, "GMPpool32", strategy_),
      GMPpool64(10005, "GMPpool64", strategy_),
      GMPpool128(10006, "GMPpool128", strategy_),
      GMPpool256(10007, "GMPpool256", strategy_),
      GMPpool512(10008, "GMPpool512", strategy_),
      GMPpool1024(10009, "GMPpool1024", strategy_),
      GMPpool2048(10010, "GMPpool2048", strategy_),
      GMPpool4096(10011, "GMPpool4096", strategy_),
      GMPpool8192(10012, "GMPpool8192", strategy_),
      GMPpool16K(10013, "GMPpool16K", strategy_),
      GMPpool32K(10014, "GMPpool32K", strategy_),
      GMPpool64K(10015, "GMPpool64K", strategy_),
      GMPpool128K(10016, "GMPpool128K", strategy_),
      GMPpool256K(10017, "GMPpool256K", strategy_),
      GMPpool512K(10018, "GMPpool512K", strategy_),
      GMPpool1M(10019, "GMPpool1M", strategy_) {}

void *MemMgr::getMem(size_t size_) {
#ifdef _GMP_DEBUG_
    fprintf(stderr, "requested size: %d\n", size_);
#endif

    void *ptr = 0;

    if (size_ <= 4) {
        ptr = GMPpool4.get();
        if (!ptr)
            dump();
        return ptr;
    }
    if (size_ <= 8) {
        ptr = GMPpool8.get();
        if (!ptr)
            dump();
        return ptr;
    }
    if (size_ <= 16) {
        ptr = GMPpool16.get();
        if (!ptr)
            dump();
        return ptr;
    }
    if (size_ <= 32) {
        ptr = GMPpool32.get();
        if (!ptr)
            dump();
        return ptr;
    }
    if (size_ <= 64) {
        ptr = GMPpool64.get();
        if (!ptr)
            dump();
        return ptr;
    }
    if (size_ <= 128) {
        ptr = GMPpool128.get();
        if (!ptr)
            dump();
        return ptr;
    }
    if (size_ <= 256) {
        ptr = GMPpool256.get();
        if (!ptr)
            dump();
        return ptr;
    }
    if (size_ <= 512) {
        ptr = GMPpool512.get();
        if (!ptr)
            dump();
        return ptr;
    }
    if (size_ <= 1024) {
        ptr = GMPpool1024.get();
        if (!ptr)
            dump();
        return ptr;
    }
    if (size_ <= 2048) {
        ptr = GMPpool2048.get();
        if (!ptr)
            dump();
        return ptr;
    }
    if (size_ <= 4096) {
        ptr = GMPpool4096.get();
        if (!ptr)
            dump();
        return ptr;
    }
    if (size_ <= 8192) {
        ptr = GMPpool8192.get();
        if (!ptr)
            dump();
        return ptr;
    }
    if (size_ <= 16384) {
        ptr = GMPpool16K.get();
        if (!ptr)
            dump();
        return ptr;
    }
    if (size_ <= 32768) {
        ptr = GMPpool32K.get();
        if (!ptr)
            dump();
        return ptr;
    }
    if (size_ <= 65536) {
        ptr = GMPpool64K.get();
        if (!ptr)
            dump();
        return ptr;
    }
    if (size_ <= 131072) {
        ptr = GMPpool128K.get();
        if (!ptr)
            dump();
        return ptr;
    }
    if (size_ <= 262144) {
        ptr = GMPpool256K.get();
        if (!ptr)
            dump();
        return ptr;
    }
    if (size_ <= 524288) {
        ptr = GMPpool512K.get();
        if (!ptr)
            dump();
        return ptr;
    }
    if (size_ <= 1048576) {
        ptr = GMPpool1M.get();
        if (!ptr)
            dump();
        return ptr;
    }

    ptr = malloc(size_);
    std::cerr << "requested through malloc, size[" << size_ << "]" << std::endl;
    return ptr;
}

void MemMgr::putMem(void *ptr) {
#ifdef _GMP_DEBUG_
    fprintf(stderr, "released ptr: %x\n", ptr);
#endif

    // get the pool number and chunk size out of the stamp
    unsigned int stamp = *((unsigned int *)((char *)ptr - OBJ_PADDING));
    unsigned int chSize = stamp & BUCKET_SIZE_MASK;
    unsigned short poolNo = (stamp >> BUCKET_NUMBER_SHIFT) & BUCKET_NUMBER_MASK;

    switch ((unsigned int)chSize) {
    case 4:
        GMPpool4.buckets[poolNo].put(ptr);
        break;
    case 8:
        GMPpool8.buckets[poolNo].put(ptr);
        break;
    case 16:
        GMPpool16.buckets[poolNo].put(ptr);
        break;
    case 32:
        GMPpool32.buckets[poolNo].put(ptr);
        break;
    case 64:
        GMPpool64.buckets[poolNo].put(ptr);
        break;
    case 128:
        GMPpool128.buckets[poolNo].put(ptr);
        break;
    case 256:
        GMPpool256.buckets[poolNo].put(ptr);
        break;
    case 512:
        GMPpool512.buckets[poolNo].put(ptr);
        break;
    case 1024:
        GMPpool1024.buckets[poolNo].put(ptr);
        break;
    case 2048:
        GMPpool2048.buckets[poolNo].put(ptr);
        break;
    case 4096:
        GMPpool4096.buckets[poolNo].put(ptr);
        break;
    case 8192:
        GMPpool8192.buckets[poolNo].put(ptr);
        break;
    case 16384:
        GMPpool16K.buckets[poolNo].put(ptr);
        break;
    case 32768:
        GMPpool32K.buckets[poolNo].put(ptr);
        break;
    case 65536:
        GMPpool64K.buckets[poolNo].put(ptr);
        break;
    case 131072:
        GMPpool128K.buckets[poolNo].put(ptr);
        break;
    case 262144:
        GMPpool256K.buckets[poolNo].put(ptr);
        break;
    case 524288:
        GMPpool512K.buckets[poolNo].put(ptr);
        break;
    case 1048576:
        GMPpool1M.buckets[poolNo].put(ptr);
        break;
    default:
        std::cerr << "not my ptr[" << ptr << "] poolNo["
                  << static_cast<int>(poolNo) << "] chSize["
                  << static_cast<int>(chSize) << "]" << std::endl;
        free(ptr);
    }
}

void MemMgr::engage() { started = true; }

void MemMgr::dump() {}
} // namespace DaqDB
