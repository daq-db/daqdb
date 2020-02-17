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

#include "DefaultAllocStrategy.h"

namespace DaqDB {

DefaultAllocStrategy::DefaultAllocStrategy(const DefaultAllocStrategy &right) {}

AllocStrategy *DefaultAllocStrategy::copy() const {
    return new DefaultAllocStrategy(*this);
}

unsigned int DefaultAllocStrategy::preallocateCount() const {
    return defaultPreallocCount;
}

unsigned int DefaultAllocStrategy::incrementQuant() const {
    return defaultIncCount;
}

unsigned int DefaultAllocStrategy::decrementQuant() const {
    return defaultDecCount;
}

unsigned int DefaultAllocStrategy::minIncQuant() const {
    return defaultMinIncQuant;
}

unsigned int DefaultAllocStrategy::maxIncQuant() const {
    return defaultMaxIncQuant;
}

unsigned int DefaultAllocStrategy::minDecQuant() const {
    return defaultMinDecQuant;
}

unsigned int DefaultAllocStrategy::maxDecQuant() const {
    return defaultMaxDecQuant;
}
} // namespace DaqDB
