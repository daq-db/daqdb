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

#include <atomic>
#include <cstdint>
#include <thread>

#include "spdk/bdev.h"
#include "spdk/env.h"
#include "spdk/event.h"
#include "spdk/io_channel.h"
#include "spdk/queue.h"

#include <functional>

#include "OffloadRqstPooler.h"

namespace DaqDB {

using OffloadReactorShutdownCallback = std::function<void()>;

enum class ReactorState : std::uint8_t {
    INIT_INPROGRESS = 0,
    READY = 1,
    ERROR = 2,
    STOPPED = 3
};

void reactor_start_clb(void *offload_reactor, void *arg2);
int reactor_pooler_fn(void *offload_reactor);

class OffloadReactor {
  public:
    OffloadReactor(const size_t cpuCore = 0, const std::string &spdkConfigFile = "",
                   OffloadReactorShutdownCallback clb = nullptr);
    virtual ~OffloadReactor();

    void RegisterPooler(OffloadRqstPooler *offloadPooler);
    void StartThread();

    std::atomic<ReactorState> state;
    std::vector<OffloadRqstPooler *> rqstPoolers;

    BdevContext bdevContext;

  private:
    void _ThreadMain(void);

    std::string _spdkConfigFile;
    std::unique_ptr<spdk_app_opts> _spdkAppOpts;

    OffloadReactorShutdownCallback _shutdownClb;
    std::thread *_thread;
    size_t _cpuCore = 0;
};
}
