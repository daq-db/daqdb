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

#include "DefaultAllocStrategy.h"
#include "GeneralPool.h"
#include "GlobalMemoryAlloc.h"

namespace MemMgmt {
class MemMgr {
  public:
    struct Size4 {
        char filler[4];
    };
    struct Size8 {
        char filler[8];
    };
    struct Size16 {
        char filler[16];
    };
    struct Size32 {
        char filler[32];
    };
    struct Size64 {
        char filler[64];
    };
    struct Size128 {
        char filler[128];
    };
    struct Size256 {
        char filler[256];
    };
    struct Size512 {
        char filler[512];
    };
    struct Size1024 {
        char filler[1024];
    };
    struct Size2048 {
        char filler[2048];
    };
    struct Size4096 {
        char filler[4096];
    };
    struct Size8192 {
        char filler[8192];
    };
    struct Size16384 {
        char filler[16384];
    };
    struct Size32768 {
        char filler[32768];
    };
    struct Size65536 {
        char filler[65536];
    };
    struct Size131072 {
        char filler[131072];
    };
    struct Size262144 {
        char filler[262144];
    };
    struct Size524288 {
        char filler[524288];
    };
    struct Size1048576 {
        char filler[1048576];
    };

    MemMgr(const AllocStrategy &strategy_ = DefaultAllocStrategy());
    virtual ~MemMgr();
    void *getMem(size_t size_);
    void putMem(void *ptr);
    static void engage();
    void dump();
    static bool started;

  private:
    MemMgr(const MemMgr &right);
    MemMgr &operator=(const MemMgr &right);
    GeneralPool<void, GlobalMemoryAlloc<Size4>> GMPpool4;
    GeneralPool<void, GlobalMemoryAlloc<Size8>> GMPpool8;
    GeneralPool<void, GlobalMemoryAlloc<Size16>> GMPpool16;
    GeneralPool<void, GlobalMemoryAlloc<Size32>> GMPpool32;
    GeneralPool<void, GlobalMemoryAlloc<Size64>> GMPpool64;
    GeneralPool<void, GlobalMemoryAlloc<Size128>> GMPpool128;
    GeneralPool<void, GlobalMemoryAlloc<Size256>> GMPpool256;
    GeneralPool<void, GlobalMemoryAlloc<Size512>> GMPpool512;
    GeneralPool<void, GlobalMemoryAlloc<Size1024>> GMPpool1024;
    GeneralPool<void, GlobalMemoryAlloc<Size2048>> GMPpool2048;
    GeneralPool<void, GlobalMemoryAlloc<Size4096>> GMPpool4096;
    GeneralPool<void, GlobalMemoryAlloc<Size8192>> GMPpool8192;
    GeneralPool<void, GlobalMemoryAlloc<Size16384>> GMPpool16K;
    GeneralPool<void, GlobalMemoryAlloc<Size32768>> GMPpool32K;
    GeneralPool<void, GlobalMemoryAlloc<Size65536>> GMPpool64K;
    GeneralPool<void, GlobalMemoryAlloc<Size131072>> GMPpool128K;
    GeneralPool<void, GlobalMemoryAlloc<Size262144>> GMPpool256K;
    GeneralPool<void, GlobalMemoryAlloc<Size524288>> GMPpool512K;
    GeneralPool<void, GlobalMemoryAlloc<Size1048576>> GMPpool1M;
};

inline MemMgr::~MemMgr() {}

} // namespace MemMgmt

#ifdef _MM_GMP_ON_
void *operator new(size_t size);
void *operator new[](size_t size);
void operator delete(void *ptr)__THROWSPEC_NULL;
void operator delete[](void *ptr) __THROWSPEC_NULL;

extern MemMgmt::MemMgr theMemMgr;

void *operator new(size_t size) {
    void *ptr = 0;
    if (MemMgmt::MemMgr::started)
        ptr = theMemMgr.getMem(size);
    else
        ptr = malloc(size);

#ifdef _GMP_DEBUG_
    fprintf(stderr, "new %d %x\n ", size, ptr);
#endif

    return ptr;
}

void *operator new[](size_t size) {
    void *ptr = 0;
    if (MemMgmt::MemMgr::started)
        ptr = theMemMgr.getMem(size);
    else
        ptr = malloc(size);

#ifdef _GMP_DEBUG_
    fprintf(stderr, "new %d %x\n ", size, ptr);
#endif

    return ptr;
}

void operator delete(void *ptr)__THROWSPEC_NULL {
#ifdef _GMP_DEBUG_
    fprintf(stderr, "delete %x\n ", ptr);
#endif

    // sanity check
    if (!ptr)
        return;

    if (MemMgmt::MemMgr::started)
        theMemMgr.putMem(ptr);
    else
        free(ptr);
}

void operator delete[](void *ptr) __THROWSPEC_NULL {
#ifdef _GMP_DEBUG_
    fprintf(stderr, "delete %x\n ", ptr);
#endif

    // sanity check
    if (!ptr)
        return;

    if (MemMgmt::MemMgr::started)
        theMemMgr.putMem(ptr);
    else
        free(ptr);
}

#endif
