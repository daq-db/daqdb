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
