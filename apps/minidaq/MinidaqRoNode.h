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

#include "MinidaqNode.h"

namespace DaqDB {

class MinidaqRoNode : public MinidaqNode {
  public:
    explicit MinidaqRoNode(KVStoreBase *kvs);
    virtual ~MinidaqRoNode();

    void SetFragmentSize(size_t s);
    void SetSubdetectorId(int id);

  protected:
    void _Task(Key &&key, std::atomic<std::uint64_t> &cnt,
               std::atomic<std::uint64_t> &cntErr);
    void _Setup(int executorId);
    Key _NextKey();
    std::string _GetType();

    size_t _fSize = 0;
    int _id = 0;
    static thread_local int _eventId;
    static thread_local constexpr char _data_buffer[100000] = " ";
};
}
