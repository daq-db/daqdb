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

namespace DaqDB {
class AllocStrategy {
  public:
    AllocStrategy() = default;
    virtual ~AllocStrategy() = default;
    virtual AllocStrategy *copy() const = 0;
    virtual unsigned int
    preallocateCount() const = 0; // # of preallocated objects per pool
    virtual unsigned int
    incrementQuant() const = 0; // Alloc increment when pool count is 0
    virtual unsigned int
    decrementQuant() const = 0; // Dealloc decrement when count > prealloc
    virtual unsigned int
    minIncQuant() const = 0; // Min increment in one go, smooths out growth
    virtual unsigned int
    maxIncQuant() const = 0; // Max increment in one go, smooths out growth
    virtual unsigned int
    minDecQuant() const = 0; // Min decrement in one go, smooths out decrese
    virtual unsigned int
    maxDecQuant() const = 0; // Max decrement in one go, smoots out decrease

  private:
    AllocStrategy(const AllocStrategy &right);
    AllocStrategy &operator=(const AllocStrategy &right);
};
} // namespace DaqDB
