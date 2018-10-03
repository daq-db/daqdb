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

#include <sstream>
#include <iostream>
#include <pthread.h>
#include <string>

#include <boost/filesystem.hpp>

#include "OffloadReactor.h"
#include <Logger.h>

#define OFFLOAD_POOLER_INTERVAL_MICR_SEC 100

namespace DaqDB {

static void set_reactor_error_state(OffloadReactor *reactor, std::string msg) {
        FOG_DEBUG(msg);
        reactor->state = ReactorState::REACTOR_ERROR;
}

void reactor_start_clb(void *ctx, void *arg) {
    spdk_unaffinitize_thread();

    // @TODO jradtke: replace with spdk_bdev_get_by_name
    auto *reactor = reinterpret_cast<OffloadReactor *>(ctx);
    auto bdev = spdk_bdev_first();
    if (!bdev) {
        set_reactor_error_state(reactor, "No NVMe devices detected");
        return;
    }
    reactor->bdevCtx.bdev = bdev;

    auto rc =
        spdk_bdev_open(bdev, true, NULL, NULL, &reactor->bdevCtx.bdev_desc);
    if (rc) {
        set_reactor_error_state(reactor, "Open BDEV failed with error code [" +
                                             std::to_string(rc) + "]");
        return;
    }

    reactor->bdevCtx.io_channel =
        spdk_bdev_get_io_channel(reactor->bdevCtx.bdev_desc);
    if (!reactor->bdevCtx.io_channel) {
        set_reactor_error_state(reactor, "Get io_channel failed");
        return;
    }

    reactor->bdevCtx.blk_size = spdk_bdev_get_block_size(bdev);
    reactor->bdevCtx.buf_align = spdk_bdev_get_buf_align(bdev);
    reactor->bdevCtx.blk_num = spdk_bdev_get_num_blocks(bdev);

    spdk_poller_register(reactor_pooler_fn, ctx,
                         OFFLOAD_POOLER_INTERVAL_MICR_SEC);

    reactor->state = ReactorState::REACTOR_READY;
}

int reactor_pooler_fn(void *ctx) {
    auto *reactor = reinterpret_cast<OffloadReactor *>(ctx);
    for (auto pooler : reactor->poolers) {
        pooler->Dequeue();
        pooler->Process();
    }

    return 0;
}

OffloadReactor::OffloadReactor(const size_t cpuCore, const std::string &spdkCfg,
                               ReactorShutClb clb)
    : state(ReactorState::REACTOR_INIT), _thread(nullptr), _cpuCore(cpuCore),
      _shutClb(clb), _spdkCfg(spdkCfg) {
    auto opts = new spdk_app_opts;
    spdk_app_opts_init(opts);
    opts->name = "OffloadReactor";
    opts->shm_id = 0;
    opts->max_delay_us = 1000 * 1000;
    opts->print_level = SPDK_LOG_ERROR;
    _spdkAppOpts.reset(opts);

    this->bdevCtx.bdev_desc = NULL;

    StartThread();
}

OffloadReactor::~OffloadReactor() {
    spdk_app_stop(0);
    if (_thread)
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

void OffloadReactor::RegisterPooler(OffloadPooler *pooler) {
    poolers.push_back(pooler);
}

void OffloadReactor::_ThreadMain(void) {
    int rc;

    if (boost::filesystem::exists(_spdkCfg))
        _spdkAppOpts->config_file = _spdkCfg.c_str();

    rc = spdk_app_start(_spdkAppOpts.get(), reactor_start_clb, this, NULL);

    FOG_DEBUG("SPDK reactor exit with rc=" + std::to_string(rc));
    spdk_app_fini();
    if (rc) {
        state = ReactorState::REACTOR_ERROR;
    } else {
        state = ReactorState::REACTOR_STOPPED;
    }
    _shutClb();
}

std::string OffloadReactor::printStatus() {
    std::stringstream result;

    if (state == ReactorState::REACTOR_READY) {
        result << "Offload: active";
    } else {
        result << "Offload: inactive";
    }

    return result.str();
}

}
