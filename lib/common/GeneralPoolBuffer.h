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

#include <iostream>
using namespace std;
#include <stdlib.h>
#include <string.h>

namespace DaqDB {
const unsigned int MAX_BUFFER_SLOTS =
    1024; // that many buffers we can have maximum
const unsigned int OBJ_PADDING =
    sizeof(unsigned int); // each object in memory padded with that many bytes

const unsigned int BUCKET_SIZE_MASK = 0x00FFFFFF; // bucket size mask
const unsigned int BUCKET_NUMBER_SHIFT =
    24; // bucket number is shifted by 24 bits
const unsigned int BUCKET_NUMBER_MASK = 0x000000FF; // bucket number mask

template <class T, class Alloc> class GeneralPoolBuffer {
  public:
    GeneralPoolBuffer(unsigned int bucketNum_);
    virtual ~GeneralPoolBuffer();
    static void *operator new(size_t size);
    static void operator delete(void *object);
    void rellocate(const GeneralPoolBuffer<T, Alloc> *buf_,
                   unsigned int howMany_);
    void zeroOut(unsigned int howMany_);

    T *slots[MAX_BUFFER_SLOTS];
    unsigned int bucketNum;

  private:
    GeneralPoolBuffer(const GeneralPoolBuffer<T, Alloc> &right);
    GeneralPoolBuffer<T, Alloc> &
    operator=(const GeneralPoolBuffer<T, Alloc> &right);
};

template <class T, class Alloc>
inline GeneralPoolBuffer<T, Alloc>::GeneralPoolBuffer(unsigned int bucketNum_)
    : bucketNum(bucketNum_) {
    memset((void *)&slots[0], '\0', (size_t)(MAX_BUFFER_SLOTS * sizeof(T *)));
}

template <class T, class Alloc>
inline GeneralPoolBuffer<T, Alloc>::~GeneralPoolBuffer() {}

template <class T, class Alloc>
inline void *GeneralPoolBuffer<T, Alloc>::operator new(size_t size) {
#ifndef _MM_GMP_ON_
    return malloc(size);
#else
    return malloc(size);
#endif
}

template <class T, class Alloc>
inline void GeneralPoolBuffer<T, Alloc>::operator delete(void *object) {
#ifndef _MM_GMP_ON_
    free(object);
#else
    free(object);
#endif
}

template <class T, class Alloc>
inline void
GeneralPoolBuffer<T, Alloc>::rellocate(const GeneralPoolBuffer<T, Alloc> *buf_,
                                       unsigned int howMany_) {
    memcpy((void *)&slots[0], (void *)&buf_->slots[0],
           (size_t)(howMany_ * sizeof(T *)));
}

template <class T, class Alloc>
inline void GeneralPoolBuffer<T, Alloc>::zeroOut(unsigned int howMany_) {
    memset((void *)slots, '\0', (size_t)(howMany_ * sizeof(T *)));
}

} // namespace DaqDB
