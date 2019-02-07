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
