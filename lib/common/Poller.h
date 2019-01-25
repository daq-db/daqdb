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

#include "spdk/env.h"
#include "spdk/io_channel.h"
#include "spdk/queue.h"

#define DEQUEUE_RING_LIMIT 1024

namespace DaqDB {

template <class T> class Poller {
  public:
    Poller() : requests(new T *[DEQUEUE_RING_LIMIT]) {
        rqstRing = spdk_ring_create(SPDK_RING_TYPE_MP_SC, 4096 * 4,
                                    SPDK_ENV_SOCKET_ID_ANY);
    }
    virtual ~Poller() {
        spdk_ring_free(rqstRing);
        delete[] requests;
    }

    virtual bool enqueue(T *rqst) {
        size_t count = spdk_ring_enqueue(rqstRing, (void **)&rqst, 1);
        return (count == 1);
    }
    virtual void dequeue() {
        requestCount = spdk_ring_dequeue(rqstRing, (void **)&requests[0],
                                         DEQUEUE_RING_LIMIT);
        assert(requestCount <= DEQUEUE_RING_LIMIT);
    }

    virtual void process() = 0;

    struct spdk_ring *rqstRing;
    unsigned short requestCount = 0;
    T **requests;
};

} // namespace DaqDB
