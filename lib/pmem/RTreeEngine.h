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
