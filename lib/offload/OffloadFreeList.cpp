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

#include <iostream>

#include "OffloadFreeList.h"
#include <Logger.h>

namespace DaqDB {

/*
 * Inserts a new element at the end of the queue.
 */
void OffloadFreeList::push(pool_base &pop, int64_t value) {
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

int64_t OffloadFreeList::get(pool_base &pop) {
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

void OffloadFreeList::clear(pool_base &pop) {
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
void OffloadFreeList::show(void) const {
    for (auto n = _head; n != nullptr; n = n->next)
        std::cout << n->lba << std::endl;
}

} // namespace DaqDB
