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

#include <signal.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <iostream>
#include <mutex>
#include <pthread.h>
#include <sstream>
#include <string>
#include <thread>

#include <boost/filesystem.hpp>

#include "spdk/conf.h"
#include "spdk/cpuset.h"
#include "spdk/env.h"
#include "spdk/event.h"
#include "spdk/ftl.h"
#include "spdk/log.h"
#include "spdk/queue.h"
#include "spdk/stdinc.h"
#include "spdk/thread.h"

#include <daqdb/Options.h>

#include "Poller.h"
#include "Rqst.h"
#include "SpdkBdevFactory.h"
#include "SpdkCore.h"
#include <Logger.h>
#include <RTree.h>

namespace DaqDB {

const char *SpdkCore::spdkHugepageDirname = "/mnt/huge_1GB";

SpdkCore::SpdkCore(OffloadOptions _offloadOptions)
    : state(SpdkState::SPDK_INIT), offloadOptions(_offloadOptions), poller(0),
      _spdkThread(0), _loopThread(0), _ready(false), _cpuCore(1),
      _spdkConf(offloadOptions), _spdkPoller(0) {
    removeConfFile();
    bool conf_file_ok = createConfFile();

    spBdev.reset(SpdkBdevFactory::getBdev(offloadOptions.devType));
    spBdev->enableStats(false);

    if (conf_file_ok == false) {
        if (spdkEnvInit() == false)
            state = SpdkState::SPDK_ERROR;
        else
            state = SpdkState::SPDK_READY;
        dynamic_cast<SpdkBdev *>(spBdev.get())->state =
            SpdkBdevState::SPDK_BDEV_NOT_FOUND;
    } else
        state = SpdkState::SPDK_READY;
}

SpdkCore::~SpdkCore() {
    if (_spdkThread != nullptr)
        _spdkThread->join();
}

bool SpdkCore::spdkEnvInit(void) {
    spdk_env_opts opts;
    spdk_env_opts_init(&opts);

    opts.name = SPDK_APP_ENV_NAME.c_str();
    /*
     * SPDK will use 1G huge pages when mem_size is 1024
     */
    opts.mem_size = 1024;

    opts.shm_id = 0;

    return (spdk_env_init(&opts) == 0);
}

bool SpdkCore::createConfFile(void) {
    if (!bf::exists(DEFAULT_SPDK_CONF_FILE)) {
        if (isNvmeInOptions()) {
            ofstream spdkConf(DEFAULT_SPDK_CONF_FILE, ios::out);
            if (spdkConf) {
                switch (offloadOptions.devType) {
                case OffloadDevType::BDEV:
                    assert(offloadOptions._devs.size() == 1);
                    spdkConf << "[Nvme]" << endl
                             << "  TransportID \"trtype:PCIe traddr:"
                             << offloadOptions._devs[0].nvmeAddr << "\" "
                             << offloadOptions._devs[0].nvmeName << endl;

                    spdkConf.close();
                    break;
                case OffloadDevType::JBOD:
                    spdkConf << "[Nvme]" << endl;
                    for (auto b : offloadOptions._devs) {
                        spdkConf << "  TransportID \"trtype:PCIe traddr:"
                                 << b.nvmeAddr << "\" " << b.nvmeName << endl;
                    }
                    spdkConf.close();
                    break;
                case OffloadDevType::RAID0:
                    std::cout << "RAID0 bdev configuration not supported yet"
                              << std::endl;
                    break;
                }

                DAQ_DEBUG("SPDK configuration file created");
                return true;
            } else {
                DAQ_DEBUG("Cannot create SPDK configuration file");
                return false;
            }
        } else {
            DAQ_DEBUG(
                "SPDK configuration file creation skipped - no NVMe device");
            return false;
        }
    } else {
        DAQ_DEBUG("SPDK configuration file creation skipped");
        return true;
    }
}

void SpdkCore::removeConfFile(void) {
    if (bf::exists(DEFAULT_SPDK_CONF_FILE)) {
        bf::remove(DEFAULT_SPDK_CONF_FILE);
    }
}

void SpdkCore::restoreSignals() {
    ::signal(SIGTERM, SIG_DFL);
    ::signal(SIGINT, SIG_DFL);
    ::signal(SIGSEGV, SIG_DFL);
}

/*
 * Start up Spdk, including SPDK thread, to initialize SPDK environemnt and a
 * Bdev
 */
void SpdkCore::startSpdk() {
    _spdkThread = new std::thread(&SpdkCore::_spdkThreadMain, this);
    DAQ_DEBUG("SpdkCore thread started");
    if (_cpuCore) {
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(_cpuCore, &cpuset);

        const int set_result = pthread_setaffinity_np(
            _spdkThread->native_handle(), sizeof(cpu_set_t), &cpuset);
        if (!set_result) {
            DAQ_DEBUG("SpdkCore thread affinity set on CPU core [" +
                      std::to_string(_cpuCore) + "]");
        } else {
            DAQ_DEBUG("Cannot set affinity on CPU core [" +
                      std::to_string(_cpuCore) + "] for OffloadReactor");
        }
    }
}

/*
 * Callback function called by SPDK spdk_app_start in the context of an SPDK
 * thread.
 */
void SpdkCore::spdkStart(void *arg) {
    SpdkCore *spdkCore = reinterpret_cast<SpdkCore *>(arg);
    SpdkDevice *bdev = spdkCore->spBdev.get();
    SpdkBdevCtx *bdev_c = bdev->getBdevCtx();

    bool rc = bdev->init(spdkCore->_spdkConf);
    if (rc == false) {
        DAQ_CRITICAL("Bdev init failed");
        spdkCore->signalReady();
        spdk_app_stop(-1);
        return;
    }

    bdev->setMaxQueued(bdev->getIoCacheSize(), bdev->getBlockSize());
    auto aligned = bdev->getAlignedSize(spdkCore->offloadOptions.allocUnitSize);
    bdev->setBlockNumForLba(aligned / bdev_c->blk_size);

    spdkCore->poller->initFreeList();
    bool i_rc = spdkCore->poller->init();
    if (i_rc == false) {
        DAQ_CRITICAL("Poller init failed");
        spdkCore->signalReady();
        spdk_app_stop(-1);
        return;
    }

    spdkCore->poller->setRunning(1);
    spdkCore->setSpdkPoller(
        spdk_poller_register(SpdkCore::spdkPollerFunction, spdkCore, 0));

    bdev->setReady();
    spdkCore->signalReady();
    spdkCore->restoreSignals();

    spdk_unaffinitize_thread();
    struct spdk_thread *this_thread = spdk_get_thread();
    for (;;) {
        if (spdk_thread_poll(this_thread, 0, 0) > 0)
            break;
    }
}

void SpdkCore::_spdkThreadMain(void) {
    struct spdk_app_opts daqdb_opts = {};
    spdk_app_opts_init(&daqdb_opts);
    daqdb_opts.config_file = DEFAULT_SPDK_CONF_FILE.c_str();
    daqdb_opts.name = "daqdb_nvme";

    int rc = spdk_app_start(&daqdb_opts, SpdkCore::spdkStart, this);
    if (rc) {
        DAQ_CRITICAL("Error spdk_app_start[" + std::to_string(rc) + "]");
        return;
    }
    DAQ_DEBUG("spdk_app_start[" + std::to_string(rc) + "]");
    spdk_app_fini();
}

int SpdkCore::spdkPollerFunction(void *arg) {
    SpdkCore *spdkCore = reinterpret_cast<SpdkCore *>(arg);
    Poller<OffloadRqst> *poller = spdkCore->poller;
    SpdkDevice *bdev = spdkCore->spBdev.get();

    uint32_t to_qu_cnt = 400; // bdev->canQueue();
    if (to_qu_cnt) {
        poller->dequeue(to_qu_cnt);
        poller->process();
    }

    if (poller->isOffloadRunning() == false) {
        spdk_poller_unregister(&spdkCore->_spdkPoller);
        bdev->deinit();
        spdk_app_stop(0);
        bdev->setRunning(0);
        return 1;
    }

    return 0;
}

void SpdkCore::signalReady() {
    std::unique_lock<std::mutex> lk(_syncMutex);
    _ready = true;
    _cv.notify_all();
}

bool SpdkCore::waitReady() {
    const std::chrono::milliseconds timeout(10000);
    std::unique_lock<std::mutex> lk(_syncMutex);
    _cv.wait_for(lk, timeout, [this] { return _ready; });
    return _ready;
}

} // namespace DaqDB
