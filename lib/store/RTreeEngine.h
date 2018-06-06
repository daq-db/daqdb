/**
 * Copyright 2018, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of the copyright holder nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include "FogKV/Status.h"

#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/make_persistent_array.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>
#include <string>

using std::string;
using std::to_string;

namespace FogKV {

class RTreeEngine {
  public:
    static RTreeEngine *Open(const string &engine, // open storage engine
                             const string &path,   // path to persistent pool
                             size_t size); // size used when creating pool
    static void Close(RTreeEngine *kv);    // close storage engine

    virtual string Engine() = 0;          // engine identifier
    virtual StatusCode Get(int32_t limit, // copy value to fixed-size buffer
                           const char *key, int32_t keybytes, char *value,
                           int32_t *valuebytes) = 0;
    virtual StatusCode Get(const string &key, // append value to std::string
                           string *value) = 0;
    virtual StatusCode Put(const string &key, // copy value from std::string
                           const string &value) = 0;
    virtual StatusCode Put(const char *key, int32_t keybytes, const char *value,
                           int32_t valuebytes) = 0;
    virtual StatusCode Remove(const string &key) = 0; // remove value for key
    virtual StatusCode AllocValueForKey(const string &key, size_t size,
                                        char **value) = 0;
};
}
