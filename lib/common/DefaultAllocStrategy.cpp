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

unsigned int DefaultAllocStrategy::preallocateCount() const { return 0; }

unsigned int DefaultAllocStrategy::incrementQuant() const { return 320; }

unsigned int DefaultAllocStrategy::decrementQuant() const { return 0; }

unsigned int DefaultAllocStrategy::minIncQuant() const { return 4; }

unsigned int DefaultAllocStrategy::maxIncQuant() const { return 10; }

unsigned int DefaultAllocStrategy::minDecQuant() const { return 0; }

unsigned int DefaultAllocStrategy::maxDecQuant() const { return 0; }
} // namespace DaqDB
