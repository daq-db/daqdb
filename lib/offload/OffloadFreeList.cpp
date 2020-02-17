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

void OffloadFreeList::pushBlock(pool_base &pop, int64_t value, uint32_t slot) {
    transaction::exec_tx(pop, [&] {
        auto n = make_persistent<FreeLba>();

        n->lba = value;
        n->next = nullptr;

        if (_heads[slot] == nullptr && _tails[slot] == nullptr) {
            _heads[slot] = _tails[slot] = n;
        } else {
            _tails[slot]->next = n;
            _tails[slot] = n;
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

int64_t OffloadFreeList::getBlock(pool_base &pop, int64_t *lbas,
                                  uint32_t lbasSize, uint32_t slot) {
    int64_t ret = -1;
    uint32_t cnt = 0;
    assert(slot < numSlots);
    transaction::exec_tx(pop, [&] {
        if (_heads[slot] == nullptr || maxLba == 0)
            transaction::abort(EINVAL);

        for (ret = _heads[slot]->lba;;) {
            if (ret < 0 || cnt >= lbasSize)
                break;

            lbas[cnt++] = ret;
            auto n = _heads[slot]->next;
            delete_persistent<FreeLba>(_heads[slot]);
            _heads[slot] = n;
            if (_heads[slot] == nullptr) {
                _tails[slot] = nullptr;
                break;
            }
        }

        if (cnt < lbasSize && ret < 0 && _heads[slot] != nullptr) {
            ret = abs(ret);
            if (ret <= maxLba - (slot + 1)) {
                lbas[cnt] = ret;
                for (; cnt < lbasSize; cnt++) {
                    _heads[slot]->lba = _heads[slot]->lba - (slot + 1);
                    ret = abs(_heads[slot]->lba);
                    lbas[cnt] = ret;
                    if (!ret)
                        break;
                }
            } else {
                ret = -1;
                transaction::abort(EINVAL);
            }
        }
    });

    return static_cast<int64_t>(cnt);
}

void OffloadFreeList::putBlock(pool_base &pop, int64_t *lbas, uint32_t lbasSize,
                               uint32_t slot) {
    for (uint32_t i = 0; i < lbasSize; i++)
        pushBlock(pop, lbas[i], slot);
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

void OffloadFreeList::clearBlock(pool_base &pop, uint32_t slot) {
    transaction::exec_tx(pop, [&] {
        while (_heads[slot] != nullptr) {
            auto n = _heads[slot]->next;

            delete_persistent<FreeLba>(_heads[slot]);
            _heads[slot] = n;

            if (_heads[slot] == nullptr)
                _tails[slot] = nullptr;
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

void OffloadFreeList::showBlock(uint32_t slot) const {
    for (auto n = _heads[slot]; n != nullptr; n = n->next)
        std::cout << n->lba << std::endl;
}

} // namespace DaqDB
