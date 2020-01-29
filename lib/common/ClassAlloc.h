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

void *getNewPtr(size_t siz_, size_t inc_);

namespace DaqDB {
template <class Z> class ClassAlloc {
  public:
    static Z *New(unsigned int padd_);
    static void Delete(Z *obj_);
    static unsigned int objectSize();
    static const char *getName();

  private:
    ClassAlloc(const ClassAlloc<Z> &right);
    ClassAlloc<Z> &operator=(const ClassAlloc<Z> &right);
};

template <class Z> inline Z *ClassAlloc<Z>::New(unsigned int padd_) {
#ifdef _MM_DEBUG_
#ifndef _MM_GMP_ON_
    cout << " ClassAlloc<Z>::New, padd_: " << padd_
         << " sizeof(Z): " << sizeof(Z) << endl
         << flush;
#else
    fprintf(stderr, " ClassAlloc<Z>::New, padd_: %d sizeof(Z): %d\n", padd_,
            sizeof(Z));
    fflush(stderr);
#endif
#endif

    return ::new (getNewPtr(sizeof(Z), padd_)) Z;
}

template <class Z> inline void ClassAlloc<Z>::Delete(Z *obj_) {}

template <class Z> inline unsigned int ClassAlloc<Z>::objectSize() {
    return (unsigned int)sizeof(Z);
}

template <class Z> inline const char *ClassAlloc<Z>::getName() {
    static const type_info &ti = typeid(Z);

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
