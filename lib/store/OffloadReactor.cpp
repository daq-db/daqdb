/**
 * Copyright 2018, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of the copyright holder nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "OffloadReactor.h"

#include <iostream>
#include <pthread.h>
#include <string>

#include <boost/filesystem.hpp>

#include "../debug/Logger.h"

#define OFFLOAD_POOLER_INTERVAL_MICR_SEC 100

namespace FogKV {

void reactor_start_clb(void *offload_reactor, void *arg2) {
    OffloadReactor *offloadReactor =
        reinterpret_cast<OffloadReactor *>(offload_reactor);
    spdk_unaffinitize_thread();

    // @TODO jradtke: replace with spdk_bdev_get_by_name
    offloadReactor->bdevContext.bdev = spdk_bdev_first();
    if (offloadReactor->bdevContext.bdev == nullptr) {
        FOG_DEBUG("No NVMe devices detected!");
        // @TODO jradtke: add error handling, should app be closed if no nvme?
    } else {
        int rc = 0;
        offloadReactor->bdevContext.bdev_desc = NULL;
        rc = spdk_bdev_open(offloadReactor->bdevContext.bdev, true, NULL, NULL,
                            &offloadReactor->bdevContext.bdev_desc);
        if (rc) {
            // @TODO jradtke: add error handling, as above
            FOG_DEBUG("spdk_bdev_open failed with rc = " + std::to_string(rc));
        }

        offloadReactor->bdevContext.bdev_io_channel =
            spdk_bdev_get_io_channel(offloadReactor->bdevContext.bdev_desc);
        if (offloadReactor->bdevContext.bdev_io_channel == NULL) {
            // @TODO jradtke: add error handling, as above
            FOG_DEBUG("spdk_bdev_get_io_channel failed");
        }

        offloadReactor->bdevContext.blk_size =
            spdk_bdev_get_block_size(offloadReactor->bdevContext.bdev);
        offloadReactor->bdevContext.buf_align =
            spdk_bdev_get_buf_align(offloadReactor->bdevContext.bdev);
        offloadReactor->bdevContext.blk_num =
            spdk_bdev_get_num_blocks(offloadReactor->bdevContext.bdev);
    }

    spdk_poller_register(reactor_pooler_fn, offload_reactor,
                         OFFLOAD_POOLER_INTERVAL_MICR_SEC);
    offloadReactor->isRunning = 1;
}

int reactor_pooler_fn(void *offload_reactor) {
    OffloadReactor *offloadReactor =
        reinterpret_cast<OffloadReactor *>(offload_reactor);
    for (auto pooler : offloadReactor->rqstPoolers) {
        pooler->DequeueMsg();
        pooler->ProcessMsg();
    }

    return 0;
}

OffloadReactor::OffloadReactor(const size_t cpuCore, std::string spdkConfigFile,
                               OffloadReactorShutdownCallback clb)
    : isRunning(0), _thread(nullptr), _cpuCore(cpuCore), _shutdownClb(clb),
      _spdkConfigFile(spdkConfigFile) {
    auto opts = new spdk_app_opts;
    spdk_app_opts_init(opts);
    opts->name = "OffloadReactor";
    opts->shm_id = 0;
    opts->max_delay_us = 1000 * 1000;
    opts->print_level = SPDK_LOG_ERROR;
    _spdkAppOpts.reset(opts);

    StartThread();
}

OffloadReactor::~OffloadReactor() {
    spdk_app_stop(0);
    if (_thread != nullptr)
        _thread->join();
}

void OffloadReactor::StartThread() {
    _thread = new std::thread(&OffloadReactor::_ThreadMain, this);

    if (_cpuCore) {
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(_cpuCore, &cpuset);

        const int set_result = pthread_setaffinity_np(
            _thread->native_handle(), sizeof(cpu_set_t), &cpuset);
        if (set_result == 0) {
            FOG_DEBUG("Started OffloadReactor on CPU core [" +
                      std::to_string(_cpuCore) + "]");
        } else {
            FOG_DEBUG("Cannot set affinity on CPU core [" +
                      std::to_string(_cpuCore) + "] for OffloadReactor");
        }
    }
}

void OffloadReactor::RegisterPooler(OffloadRqstPooler *offloadPooler) {
    rqstPoolers.push_back(offloadPooler);
}

void OffloadReactor::_ThreadMain(void) {
    int rc;

    if (boost::filesystem::exists(_spdkConfigFile))
        _spdkAppOpts->config_file = _spdkConfigFile.c_str();

    rc = spdk_app_start(_spdkAppOpts.get(), reactor_start_clb, this, NULL);
    isRunning = 0;
    FOG_DEBUG("SPDK reactor exit with rc=" + std::to_string(rc));

    spdk_app_fini();
    _shutdownClb();
}
} // namespace FogKV
