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

DaqDB::Value allocValue(DaqDB::KVStoreBase *kvs, const uint64_t id,
                        const std::string &value);

DaqDB::Value allocValue(DaqDB::KVStoreBase *kvs, const DaqDB::Key &key,
                        const std::string &value);

const std::string generateValueStr(const size_t valueSize);

DaqDB::Key allocKey(DaqDB::KVStoreBase *kvs, const uint64_t id);

bool checkValue(const std::string &expectedValue, DaqDB::Value *value);

const std::string keyToStr(DaqDB::Key &key);

const std::string keyToStr(const char *key);

DaqDB::Value remote_get(DaqDB::KVStoreBase *kvs, const uint64_t id);

void remote_put(DaqDB::KVStoreBase *kvs, const uint64_t id,
                const std::string &value);

bool remote_remove(DaqDB::KVStoreBase *kvs, const uint64_t id);
