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

#include <PrimaryKeyBase.h>

namespace DaqDB {

PrimaryKeyBase::PrimaryKeyBase(const DaqDB::Options &Options) {}

PrimaryKeyBase::~PrimaryKeyBase() {}

Key PrimaryKeyBase::DequeueNext() {
    throw OperationFailedException(NOT_SUPPORTED);
}

void PrimaryKeyBase::EnqueueNext(Key &&Key) {}

bool PrimaryKeyBase::IsLocal(const Key &key) { return false; }

} // namespace DaqDB
