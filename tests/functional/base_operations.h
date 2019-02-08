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
void daqdb_put(DaqDB::KVStoreBase *kvs, DaqDB::Key &&key, DaqDB::Value &val);

void daqdb_update(DaqDB::KVStoreBase *kvs, DaqDB::Key &key, DaqDB::Value &val,
                  const DaqDB::UpdateOptions &options);

void daqdb_offload(DaqDB::KVStoreBase *kvs, DaqDB::Key &key);

void daqdb_async_offload(DaqDB::KVStoreBase *kvs, DaqDB::Key &key,
                         DaqDB::KVStoreBase::KVStoreBaseCallback cb);

bool daqdb_remove(DaqDB::KVStoreBase *kvs, DaqDB::Key &key);

void daqdb_async_get(DaqDB::KVStoreBase *kvs, const DaqDB::Key &key,
                     DaqDB::KVStoreBase::KVStoreBaseCallback cb);

void daqdb_async_put(DaqDB::KVStoreBase *kvs, DaqDB::Key &&key,
                     DaqDB::Value &val,
                     DaqDB::KVStoreBase::KVStoreBaseCallback cb);

void daqdb_async_update(DaqDB::KVStoreBase *kvs, DaqDB::Key &key,
                        DaqDB::Value &val,
                        DaqDB::KVStoreBase::KVStoreBaseCallback cb);
