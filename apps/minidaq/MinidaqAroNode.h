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

#include "MinidaqRoNode.h"

namespace DaqDB {

class MinidaqAroNode : public MinidaqRoNode {
  public:
    explicit MinidaqAroNode(KVStoreBase *kvs);
    ~MinidaqAroNode();

  protected:
    void _Task(Key &&key, std::atomic<std::uint64_t> &cnt,
               std::atomic<std::uint64_t> &cntErr);
    std::string _GetType();
};
}
