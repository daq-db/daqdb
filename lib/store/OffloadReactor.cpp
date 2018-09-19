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

#include "OffloadReactor.h"

#include <iostream>
#include <pthread.h>
#include <string>

#include <boost/filesystem.hpp>

#include "../debug/Logger.h"

#define OFFLOAD_POOLER_INTERVAL_MICR_SEC 100

namespace DaqDB {

void reactor_start_clb(void *offload_reactor, void *arg2) {
    OffloadReactor *offloadReactor =
        reinterpret_cast<OffloadReactor *>(offload_reactor);
    spdk_unaffinitize_thread();

    // @TODO jradtke: replace with spdk_bdev_get_by_name
    offloadReactor->bdevContext.bdev = spdk_bdev_first();
    if (offloadReactor->bdevContext.bdev == nullptr) {
        FOG_DEBUG("No NVMe devices detected!");
        offloadReactor->state = ReactorState::ERROR;
        return;
    } else {
        int rc = 0;
        offloadReactor->bdevContext.bdev_desc = NULL;
        rc = spdk_bdev_open(offloadReactor->bdevContext.bdev, true, NULL, NULL,
                            &offloadReactor->bdevContext.bdev_desc);
        if (rc) {
            FOG_DEBUG("Open BDEV failed with error code [" +
                      std::to_string(rc) + "]");
            offloadReactor->state = ReactorState::ERROR;
            return;
        }

        offloadReactor->bdevContext.bdev_io_channel =
            spdk_bdev_get_io_channel(offloadReactor->bdevContext.bdev_desc);
        if (offloadReactor->bdevContext.bdev_io_channel == NULL) {
            FOG_DEBUG("Get io_channel failed");
            offloadReactor->state = ReactorState::ERROR;
            return;
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

    offloadReactor->state = ReactorState::READY;
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

OffloadReactor::OffloadReactor(const size_t cpuCore, const std::string &spdkConfigFile,
                               OffloadReactorShutdownCallback clb)
    : state(ReactorState::INIT_INPROGRESS), _thread(nullptr), _cpuCore(cpuCore),
      _shutdownClb(clb), _spdkConfigFile(spdkConfigFile) {
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

    FOG_DEBUG("SPDK reactor exit with rc=" + std::to_string(rc));
    spdk_app_fini();
    if (rc == 0) {
        state = ReactorState::STOPPED;
    } else {
        state = ReactorState::ERROR;
    }
    _shutdownClb();
}
}
