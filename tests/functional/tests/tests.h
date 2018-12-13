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

#include <daqdb/KVStoreBase.h>

bool testSyncOperations(DaqDB::KVStoreBase *kvs);
bool testAsyncOperations(DaqDB::KVStoreBase *kvs);
bool testSyncOffloadOperations(DaqDB::KVStoreBase *kvs);
bool testAsyncOffloadOperations(DaqDB::KVStoreBase *kvs);
bool testDhtConnect(DaqDB::KVStoreBase *kvs);
bool testValueSizes(DaqDB::KVStoreBase *kvs);
