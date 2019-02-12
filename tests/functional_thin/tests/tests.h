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

#include "debug.h"
#include <daqdb/KVStoreBase.h>

/**
 * Verifies connection to remote DAQDB node (defined in configuration file)
 */
bool testRemotePeerConnect(DaqDB::KVStoreBase *kvs, DaqDB::Options *options);

/**
 * Verifies single PUT and GET operation performed on remote DAQDB peer
 * node.
 */
bool testPutGetSequence(DaqDB::KVStoreBase *kvs, DaqDB::Options *options);

/**
 * Verifies various value sizes for single PUT and GET operation performed on
 * remote DAQDB peer.
 */
bool testValueSizes(DaqDB::KVStoreBase *kvs, DaqDB::Options *options);

bool testMultithredingPutGet(DaqDB::KVStoreBase *kvs, DaqDB::Options *options);
