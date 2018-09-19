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

#include <iostream>

#include "../debug/Logger.h"
#include "OffloadFreeList.h"

namespace DaqDB {

OffloadFreeList::OffloadFreeList() {}

OffloadFreeList::~OffloadFreeList() {}

/*
 * Inserts a new element at the end of the queue.
 */
void OffloadFreeList::Push(pool_base &pop, int64_t value) {
    transaction::exec_tx(pop, [&] {
        auto n = make_persistent<FreeLba>();

        n->lba = value;
        n->next = nullptr;

        if (_head == nullptr && _tail == nullptr) {
            _head = _tail = n;
        } else {
            _tail->next = n;
            _tail = n;
        }
    });
}

int64_t OffloadFreeList::GetFreeLba(pool_base &pop) {
    int64_t ret = -1;
    transaction::exec_tx(pop, [&] {
        if (_head == nullptr || maxLba == 0)
            transaction::abort(EINVAL);

        ret = _head->lba;

        if (ret >= 0) {
            auto n = _head->next;

            delete_persistent<FreeLba>(_head);
            _head = n;

            if (_head == nullptr)
                _tail = nullptr;
        } else {
            ret = abs(ret);
            if (ret < maxLba - 1) {
                _head->lba = _head->lba - 1;
            } else if (ret == maxLba - 1) {
                _head->lba = 0;
            } else {
                ret = -1;
                transaction::abort(EINVAL);
            }
        }
    });

    return ret;
}

void OffloadFreeList::Clear(pool_base &pop) {
    transaction::exec_tx(pop, [&] {
        while (_head != nullptr) {
            auto n = _head->next;

            delete_persistent<FreeLba>(_head);
            _head = n;

            if (_head == nullptr)
                _tail = nullptr;
        }
    });
}

/*
 * Prints the entire contents of the queue.
 */
void OffloadFreeList::Show(void) const {
    for (auto n = _head; n != nullptr; n = n->next)
        std::cout << n->lba << std::endl;
}

}
