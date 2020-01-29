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

#include "ClassAlloc.h"
#include <stdlib.h>

void *getNewPtr(size_t size_, size_t inc_) {
#ifdef _MM_DEBUG_
#ifndef _MM_GMP_ON_
    std::cout << "::new(size_t size_, size_t inc_) "
              << " size_: " << size_ << " inc_: " << inc_ << endl
              << flush;
#else
    fprintf(stderr, "::new(size_t size_, size_t inc_) size_: %d inc_: %d\n",
            size_, inc_);
    fflush(stderr);
#endif
#endif

#ifndef _MM_GMP_ON_
    return ::new char[size_ + inc_];
#else
    return (char *)::malloc(size_ + inc_) + inc_;
#endif
}
