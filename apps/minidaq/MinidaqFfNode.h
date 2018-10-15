/**
 * Copyright 2018 Intel Corporation.
 *
 * This software and the related documents are Intel copyrighted materials,
 * and your use of them is governed by the express license under which they
 * were provided to you (Intel OBL Internal Use License).
 * Unless the License provides otherwise, you may not use, modify, copy,
 * publish, distribute, disclose or transmit this software or the related
 * documents without Intel's prior written permission.
 *
 * This software and the related documents are provided as is, with no
 * express or implied warranties, other than those that are expressly
 * stated in the License.
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
    void SetDelay(int d);

  protected:
    void _Task(MinidaqKey &key, std::atomic<std::uint64_t> &cnt,
               std::atomic<std::uint64_t> &cntErr);
    void _Setup(int executorId, MinidaqKey &key);
    void _NextKey(MinidaqKey &key);
    std::string _GetType();
    bool _Accept();
    int _PickSubdetector();
    int _PickNFragments();

    int _baseId = 0;
    int _nSubdetectors = 0;
    int _delay_us = 0;
    int _maxRetries = 100000;

  private:
    double _acceptLevel = 1.0; // desired acceptance level for filtering nodes
};
}
