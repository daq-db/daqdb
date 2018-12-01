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

#include "debug.h"
#include <daqdb/KVStoreBase.h>

/**
 * Verifies connection to remote DAQDB node (defined in configuration file)
 */
bool testRemotePeerConnect(DaqDB::KVStoreBase *kvs);

/**
 * Verifies single PUT and GET operation performed on remote DAQDB peer
 * node.
 */
bool testPutGetSequence(DaqDB::KVStoreBase *kvs);

/**
 * Verifies various value sizes for single PUT and GET operation performed on
 * remote DAQDB peer.
 */
bool testValueSizes(DaqDB::KVStoreBase *kvs);
