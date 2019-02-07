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

class MinidaqFfNode : public MinidaqNode {
  public:
    explicit MinidaqFfNode(KVStoreBase *kvs);
    virtual ~MinidaqFfNode();

    void SetSubdetectors(int n);
    void SetBaseSubdetectorId(int id);
    void SetAcceptLevel(double p);
    void SetRetryDelay(int d);

  protected:
    void _Task(Key &&key, std::atomic<std::uint64_t> &cnt,
               std::atomic<std::uint64_t> &cntErr);
    void _Setup(int executorId);
    Key _NextKey();
    std::string _GetType();
    bool _Accept();
    int _PickSubdetector();
    int _PickNFragments();

    int _baseId = 0;
    int _nSubdetectors = 0;
    int _delay_us = 100;
    int _maxRetries = 100;

  private:
    double _acceptLevel = 1.0; // desired acceptance level for filtering nodes
};
}
