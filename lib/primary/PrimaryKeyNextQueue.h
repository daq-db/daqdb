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

#include <PrimaryKeyBase.h>
#include <SpdkCore.h>
#include <daqdb/Key.h>
#include <daqdb/Options.h>

namespace DaqDB {

class PrimaryKeyNextQueue : public DaqDB::PrimaryKeyBase {
  public:
    PrimaryKeyNextQueue(const DaqDB::Options &Options);
    virtual ~PrimaryKeyNextQueue();
    Key DequeueNext();
    void EnqueueNext(Key &&key);

  private:
    char *_CreatePKeyBuff(char *srcKeyBuff);

    struct spdk_ring *_readyKeys;
    size_t _keySize;
    size_t _pKeySize;
    size_t _pKeyOffset;
};

} // namespace DaqDB
