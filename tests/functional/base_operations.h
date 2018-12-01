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

#include <asio/io_service.hpp>

#include <daqdb/KVStoreBase.h>
#include <daqdb/Key.h>
#include <daqdb/Options.h>
#include <daqdb/Status.h>
#include <daqdb/Value.h>

DaqDB::Value allocValue(DaqDB::KVStoreBase *kvs, const DaqDB::Key &key,
                        const std::string &value);
DaqDB::Value allocAndFillValue(DaqDB::KVStoreBase *kvs, const DaqDB::Key &key,
                               const size_t valueSize);

DaqDB::Key strToKey(DaqDB::KVStoreBase *kvs, const std::string &key);

DaqDB::Value daqdb_get(DaqDB::KVStoreBase *kvs, const DaqDB::Key &key);
void daqdb_put(DaqDB::KVStoreBase *kvs, DaqDB::Key &key, DaqDB::Value &val);

void daqdb_update(DaqDB::KVStoreBase *kvs, DaqDB::Key &key, DaqDB::Value &val,
                  const DaqDB::UpdateOptions &options);

void daqdb_offload(DaqDB::KVStoreBase *kvs, DaqDB::Key &key);

void daqdb_async_offload(DaqDB::KVStoreBase *kvs, DaqDB::Key &key,
                         DaqDB::KVStoreBase::KVStoreBaseCallback cb);

bool daqdb_remove(DaqDB::KVStoreBase *kvs, DaqDB::Key &key);

void daqdb_async_get(DaqDB::KVStoreBase *kvs, const DaqDB::Key &key,
                     DaqDB::KVStoreBase::KVStoreBaseCallback cb);

void daqdb_async_put(DaqDB::KVStoreBase *kvs, DaqDB::Key &key,
                     DaqDB::Value &val,
                     DaqDB::KVStoreBase::KVStoreBaseCallback cb);

void daqdb_async_update(DaqDB::KVStoreBase *kvs, DaqDB::Key &key,
                        DaqDB::Value &val,
                        DaqDB::KVStoreBase::KVStoreBaseCallback cb);
