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

#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

namespace DaqDB {

using pmem::obj::p;
using pmem::obj::persistent_ptr;
using pmem::obj::pool;
using pmem::obj::pool_base;
using pmem::obj::make_persistent;
using pmem::obj::delete_persistent;
using pmem::obj::transaction;

class OffloadFreeList {

    /* entry in the list */
    struct FreeLba {
        persistent_ptr<FreeLba> next;
        p<int64_t> lba;
    };

  public:
    OffloadFreeList() {};
    ~OffloadFreeList() {};

    void push(pool_base &pop, int64_t value);
    int64_t get(pool_base &pop);
    void clear(pool_base &pop);
    void show(void) const;

    uint64_t maxLba = 0;

  private:

    persistent_ptr<FreeLba> _head;
    persistent_ptr<FreeLba> _tail;
};

}
