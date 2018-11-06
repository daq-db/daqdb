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

DaqDB::Value allocValue(std::shared_ptr<DaqDB::KVStoreBase> &spKvs,
                        const DaqDB::Key &key, const std::string &value);
DaqDB::Key strToKey(std::shared_ptr<DaqDB::KVStoreBase> &spKvs,
                    const std::string &key);

DaqDB::Value daqdb_get(std::shared_ptr<DaqDB::KVStoreBase> &spKvs,
                       const DaqDB::Key &key);
void daqdb_put(std::shared_ptr<DaqDB::KVStoreBase> &spKvs, DaqDB::Key &key,
               DaqDB::Value &val);

void daqdb_update(std::shared_ptr<DaqDB::KVStoreBase> &spKvs, DaqDB::Key &key,
                  DaqDB::Value &val, const DaqDB::UpdateOptions &options);

void daqdb_offload(std::shared_ptr<DaqDB::KVStoreBase> &spKvs, DaqDB::Key &key);

void daqdb_async_offload(std::shared_ptr<DaqDB::KVStoreBase> &spKvs,
                          DaqDB::Key &key,
                          DaqDB::KVStoreBase::KVStoreBaseCallback cb);

void daqdb_remove(std::shared_ptr<DaqDB::KVStoreBase> &spKvs, DaqDB::Key &key);

void daqdb_async_get(std::shared_ptr<DaqDB::KVStoreBase> &spKvs,
                     const DaqDB::Key &key,
                     DaqDB::KVStoreBase::KVStoreBaseCallback cb);

void daqdb_async_put(std::shared_ptr<DaqDB::KVStoreBase> &spKvs,
                     DaqDB::Key &key, DaqDB::Value &val,
                     DaqDB::KVStoreBase::KVStoreBaseCallback cb);

void daqdb_async_update(std::shared_ptr<DaqDB::KVStoreBase> &spKvs,
                        DaqDB::Key &key, DaqDB::Value &val,
                        DaqDB::KVStoreBase::KVStoreBaseCallback cb);
