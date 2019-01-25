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

#include <memory>

#include "spdk/bdev.h"
#include "spdk/env.h"
#include "spdk/queue.h"
#include "spdk/thread.h"

namespace DaqDB {

/* A polling function */
struct SpdkPoller {
    spdk_poller_fn fn;
    void *arg;
    uint64_t periodMicroseconds;

    TAILQ_ENTRY(SpdkPoller) link;
};

struct SpdkTarget {
    spdk_bdev *bdev;
    spdk_bdev_desc *desc;
    spdk_io_channel *ch;

    TAILQ_ENTRY(SpdkTarget) link;
};

/* Used to pass messages between fio threads */
struct SpdkThreadMsg {
    spdk_thread_fn fn;
    void *arg;
};

struct SpdkThreadCtx {
    /* raw SPDK thread */
    spdk_thread *thread;
    /* ring for passing messages to this thread */
    spdk_ring *ring;
    /* polling timeout */
    uint64_t timeout;
    /* list of registered pollers on this thread */

    TAILQ_HEAD(, SpdkPoller) pollers;
    TAILQ_HEAD(, spdk_fio_target) targets;
};

class SpdkThread {
  public:
    SpdkThread(void);

    bool init(void);

    /**
     * Dequeues and call all messages from ring and calls registered pollers.
     *
     * @return number or dequeued messages
     */
    size_t poll(void);

  private:
    std::unique_ptr<DaqDB::SpdkThreadCtx> spCtx;
};

} // namespace DaqDB
