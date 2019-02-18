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

#include "daqdb/Status.h"

#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/make_persistent_array.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>
#include <string>

using std::string;
using std::to_string;

enum LOCATIONS { EMPTY, PMEM, DISK };

namespace DaqDB {

class RTreeEngine {
  public:
    static RTreeEngine *Open(const string &path, // path to persistent pool
                             size_t size,        // size used when creating pool
                             size_t allocUnitSize); // allocation unit size
    static void Close(RTreeEngine *kv);             // close storage engine

    virtual string Engine() = 0; // engine identifier
    virtual void Get(const char *key, int32_t keybytes, void **value,
                     size_t *size, uint8_t *location) = 0;
    virtual void Get(const char *key, void **value, size_t *size,
                     uint8_t *location) = 0;
    virtual uint64_t GetTreeSize() = 0;
    virtual uint8_t GetTreeDepth() = 0;
    virtual uint64_t GetLeafCount() = 0;
    virtual void Put(const char *key, // copy value from std::string
                     char *value) = 0;
    virtual void Put(const char *key, int32_t keybytes, const char *value,
                     int32_t valuebytes) = 0;
    virtual void Remove(const char *key) = 0; // remove value for key
    virtual void AllocValueForKey(const char *key, size_t size,
                                  char **value) = 0;
    virtual void
    AllocateIOVForKey(const char *key, uint64_t **ptr,
                      size_t size) = 0; // allocate IOV vector for given Key
    virtual void UpdateValueWrapper(const char *key, uint64_t *ptr,
                                    size_t size) = 0;
};
} // namespace DaqDB
