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

#include "AllocStrategy.h"

namespace DaqDB {
class DefaultAllocStrategy : public AllocStrategy {
  public:
    DefaultAllocStrategy() = default;
    virtual ~DefaultAllocStrategy() = default;
    virtual AllocStrategy *copy() const;
    virtual unsigned int preallocateCount() const;
    virtual unsigned int incrementQuant() const;
    virtual unsigned int decrementQuant() const;
    virtual unsigned int minIncQuant() const;
    virtual unsigned int maxIncQuant() const;
    virtual unsigned int minDecQuant() const;
    virtual unsigned int maxDecQuant() const;

  private:
    DefaultAllocStrategy(const DefaultAllocStrategy &right);
    DefaultAllocStrategy &operator=(const DefaultAllocStrategy &right);
};
} // namespace DaqDB
