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
#include <iostream>
#include <typeinfo>
using namespace std;
#include <stdlib.h>

namespace DaqDB {

/*
 * ClassMemoryAlloc template is an example class allocator with built-in
 * standard new operator User-defined classes may opt to use their own
 * allocators, implementing user-defined allocation strategies When instantiated
 * with a user-defined class, it'll be reposible for allocating and constructing
 * instances and destroying and deleting them
 */
template <class W> class ClassMemoryAlloc {
  public:
    static void *New(unsigned int padd_);
    static void Delete(void *obj_);
    static unsigned int objectSize();
    static const char *getName();

  private:
    ClassMemoryAlloc(const ClassMemoryAlloc<W> &right);
    ClassMemoryAlloc<W> &operator=(const ClassMemoryAlloc<W> &right);
};

template <class W> inline void *ClassMemoryAlloc<W>::New(unsigned int padd_) {
#ifdef _MM_DEBUG_
#ifndef _MM_GMP_ON_
    cout << " ClassMemoryAlloc<W>::New, padd_: " << padd_
         << " sizeof(W): " << sizeof(W) << endl
         << flush;
#else
    fprintf(stderr, " ClassMemoryAlloc<W>::New, padd_: %d sizeof(W): %d\n",
            padd_, sizeof(W));
    fflush(stderr);
#endif
#endif

#ifndef _MM_GMP_ON_
    return ::new char[padd_ + sizeof(W)];
#else
    return (char *)::malloc(padd_ + sizeof(W)) + padd_;
#endif
}

template <class W> inline void ClassMemoryAlloc<W>::Delete(void *obj_) {}

template <class W> inline unsigned int ClassMemoryAlloc<W>::objectSize() {
    return (unsigned int)sizeof(W);
}

template <class W> inline const char *ClassMemoryAlloc<W>::getName() {
    static const type_info &ti = typeid(W);

#ifdef _MM_DEBUG_
#ifndef _MM_GMP_ON_
    cout << "NAME: " << ti.name() << endl << flush;
#else
    fprintf(stderr, "NAME: %s\n", ti.name());
#endif
#endif

    return ti.name();
}
} // namespace DaqDB
