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

#include <PrimaryKeyEngine.h>
#include <daqdb/Key.h>
#include <daqdb/Options.h>

namespace DaqDB {

class PrimaryKeyBase : public DaqDB::PrimaryKeyEngine {
  public:
    PrimaryKeyBase(const DaqDB::Options &options);
    virtual ~PrimaryKeyBase();
    Key dequeueNext();
    void enqueueNext(Key &&key);
    bool isLocal(const Key &key);

  protected:
    size_t _keySize;
    size_t _pKeySize;
    size_t _pKeyOffset;

  private:
    uint32_t _localKeyMask;
    uint32_t _localKeyValue;
};

} // namespace DaqDB
