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

DaqDB::Key strToKey(DaqDB::KVStoreBase *kvs, const std::string &key,
                    DaqDB::KeyValAttribute attr);

DaqDB::Value remote_get(DaqDB::KVStoreBase *kvs, const DaqDB::Key &key);

void remote_put(DaqDB::KVStoreBase *kvs, DaqDB::Key &&key, DaqDB::Value &val);

bool remote_remove(DaqDB::KVStoreBase *kvs, DaqDB::Key &key);
