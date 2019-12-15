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

#include "spdk/io_channel.h"
#include "spdk/queue.h"

#define DEQUEUE_RING_LIMIT 1024

namespace DaqDB {

template <class T> class Poller {
  public:
    Poller() : rqstRing(0), requests(new T *[DEQUEUE_RING_LIMIT]) {
        rqstRing = spdk_ring_create(SPDK_RING_TYPE_MP_SC, 4096 * 4,
                                    SPDK_ENV_SOCKET_ID_ANY);
    }
    virtual ~Poller() {
        spdk_ring_free(rqstRing);
        delete[] requests;
    }

    virtual bool enqueue(T *rqst) {
        size_t count = spdk_ring_enqueue(rqstRing, (void **)&rqst, 1, 0);
        return (count == 1);
    }
    virtual void dequeue(uint32_t cnt = DEQUEUE_RING_LIMIT) {
        requestCount = spdk_ring_dequeue(
            rqstRing, (void **)&requests[0],
            cnt >= DEQUEUE_RING_LIMIT ? DEQUEUE_RING_LIMIT : cnt);
        assert(requestCount <=
               (cnt >= DEQUEUE_RING_LIMIT ? DEQUEUE_RING_LIMIT : cnt));
    }
    size_t count() { return rqstRing ? spdk_ring_count(rqstRing) : 0; }

    virtual void process() = 0;

    virtual void setRunning(int rn) {}
    virtual bool isOffloadRunning() { return false; }
    virtual void initFreeList() {}
    virtual uint32_t canQueue() { return 0; }

    struct spdk_ring *rqstRing;
    unsigned short requestCount = 0;
    T **requests;
};

} // namespace DaqDB
