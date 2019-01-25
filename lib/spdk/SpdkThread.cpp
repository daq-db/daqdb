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

#include "spdk/util.h"

#include "SpdkThread.h"
#include <Logger.h>

using namespace std;

namespace DaqDB {

const int SPDK_THREAD_RING_SIZE = 4096;
const string SPDK_THREAD_NAME = "DaqDB";
const unsigned int SPDK_POLLING_TIMEOUT = 1000000UL;

#define SPDK_FIO_POLLING_TIMEOUT 1000000UL

static void spdk_thread_send_msg_fn(spdk_thread_fn fn, void *ctx,
                                    void *thread_ctx) {
    SpdkThreadCtx *thread = reinterpret_cast<SpdkThreadCtx *>(thread_ctx);
    SpdkThreadMsg *msg;
    size_t count;

    msg = new SpdkThreadMsg();

    msg->fn = fn;
    msg->arg = ctx;

    count = spdk_ring_enqueue(thread->ring, (void **)&msg, 1);
    if (count != 1)
        DAQ_DEBUG("Unable to send message");
}

static spdk_poller *spdk_thread_start_poller_fn(void *thread_ctx,
                                                spdk_poller_fn fn, void *arg,
                                                uint64_t period_microseconds) {
    SpdkThreadCtx *thread = reinterpret_cast<SpdkThreadCtx *>(thread_ctx);
    SpdkThreadMsg *msg;
    SpdkPoller *poller = new SpdkPoller();

    poller->fn = fn;
    poller->arg = arg;
    poller->periodMicroseconds = period_microseconds;
    thread->timeout = spdk_min(thread->timeout, period_microseconds);

    TAILQ_INSERT_TAIL(&thread->pollers, poller, link);

    return reinterpret_cast<spdk_poller *>(poller);
}

static void spdk_thread_stop_poller_fn(spdk_poller *spdkPoller,
                                       void *thread_ctx) {
    SpdkPoller *poller;
    SpdkThreadCtx *thread = reinterpret_cast<SpdkThreadCtx *>(thread_ctx);
    uint64_t timeout = SPDK_POLLING_TIMEOUT;

    poller = reinterpret_cast<SpdkPoller *>(spdkPoller);

    TAILQ_REMOVE(&thread->pollers, poller, link);

    if (thread->timeout == poller->periodMicroseconds) {
        TAILQ_FOREACH(poller, &thread->pollers, link) {
            timeout = spdk_min(timeout, poller->periodMicroseconds);
        }

        thread->timeout = timeout;
    }

    delete (poller);
}

SpdkThread::SpdkThread() {
    spCtx.reset(new SpdkThreadCtx());
    TAILQ_INIT(&spCtx->pollers);
    TAILQ_INIT(&spCtx->targets);
}

bool SpdkThread::init() {
    spCtx->ring = spdk_ring_create(SPDK_RING_TYPE_MP_SC, SPDK_THREAD_RING_SIZE,
                                   SPDK_ENV_SOCKET_ID_ANY);
    if (!spCtx->ring) {
        DAQ_DEBUG("failed to allocate ring\n");
        return false;
    }

    spCtx->thread = spdk_allocate_thread(
        spdk_thread_send_msg_fn, spdk_thread_start_poller_fn,
        spdk_thread_stop_poller_fn, reinterpret_cast<void *>(spCtx.get()),
        SPDK_THREAD_NAME.c_str());

    if (!spCtx->thread) {
        spdk_ring_free(spCtx->ring);
        DAQ_DEBUG("failed to allocate thread\n");
        return false;
    }

    return true;
}

size_t SpdkThread::poll() {
    SpdkThreadMsg *msg;
    SpdkPoller *poller;
    SpdkPoller *tmpPoller;
    size_t count;

    count = spdk_ring_dequeue(spCtx->ring, reinterpret_cast<void **>(&msg), 1);
    if (count > 0) {
        msg->fn(msg->arg);
        delete (msg);
    }

    /* Call all pollers */
    TAILQ_FOREACH_SAFE(poller, &spCtx->pollers, link, tmpPoller) {
        poller->fn(poller->arg);
    }

    return count;
}

} // namespace DaqDB
