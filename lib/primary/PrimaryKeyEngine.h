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

#include <daqdb/Key.h>
#include <daqdb/Options.h>

namespace DaqDB {

class PrimaryKeyEngine {
  public:
    static PrimaryKeyEngine *open(const DaqDB::Options &options);
    virtual ~PrimaryKeyEngine();
    virtual Key dequeueNext() = 0;
    virtual void enqueueNext(Key &&key) = 0;
    virtual bool isLocal(const Key &key) = 0;
};
} // namespace DaqDB
