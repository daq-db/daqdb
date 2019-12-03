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
#include "SpdkBdev.h"
#include "SpdkBdevFactory.h"
#include "SpdkCore.h"
#include <Logger.h>
#include <RTree.h>

namespace DaqDB {

const std::string SPDK_APP_ENV_NAME = "DaqDB";
const std::string DEFAULT_SPDK_CONF_FILE = "spdk.conf";

enum class SpdkState : std::uint8_t {
    SPDK_INIT = 0,
    SPDK_READY,
    SPDK_ERROR,
    SPDK_STOPPED
};

using namespace std;
namespace bf = boost::filesystem;

template <class T> class SpdkCore {
  public:
    SpdkCore(OffloadOptions offloadOptions);

    /**
     * SPDK requires passing configuration through file.
     * This function creates temporary configuration file
     * in format supported by SPDK.
     *
     * @return false when cannot create the file or there are no NVMe devices in
     * _offloadOptions, true otherwise
     */
    bool createConfFile(void);

    void removeConfFile(void);

    inline bool isOffloadEnabled() {
        if (state == SpdkState::SPDK_READY)
            return (spBdev->spBdevCtx.state == SPDK_BDEV_READY);
        else
            return false;
    }
    bool spdkEnvInit(void);

    SpdkBdev *getBdev(void) {
        return spBdev.get();
    }

    bool isSpdkReady() {
        return state == SpdkState::SPDK_READY ? true : false;
    }

    bool isBdevFound() {
        return state == SpdkState::SPDK_READY && spBdev->state != SpdkBdevState::SPDK_BDEV_NOT_FOUND ? true : false;
    }

    void restoreSignals();

    std::atomic<SpdkState> state;

    std::unique_ptr<SpdkBdev> spBdev;
    OffloadOptions offloadOptions;
    Poller<T> *poller;

    const static char *spdkHugepageDirname;

    void signalReady();
    bool waitReady();

    void setPoller(Poller<T> *pol) { poller = pol; }

    static int spdkPollerFunction(void *arg);
    void setSpdkPoller(struct spdk_poller *spdk_poller) {
        _spdkPoller = spdk_poller;
    }

    /*
     * Callback function called by SPDK spdk_app_start in the context of an SPDK
     * thread.
     */
    static void spdkStart(void *arg);

    void startSpdk();

  private:
    std::thread *_spdkThread;
    std::thread *_loopThread;
    bool _ready;

    size_t _cpuCore;
    std::string _bdevName;

    std::string _confFile;

    std::mutex _syncMutex;
    std::condition_variable _cv;

    struct spdk_poller *_spdkPoller;

    inline bool isNvmeInOptions() {
        return !offloadOptions.nvmeAddr.empty() &&
               !offloadOptions.nvmeName.empty();
    }
    inline std::string getNvmeAddress() { return offloadOptions.nvmeAddr; }
    inline std::string getNvmeName() { return offloadOptions.nvmeName; }

    void _spdkThreadMain(void);
};

template <class T>
const char *SpdkCore<T>::spdkHugepageDirname = "/mnt/huge_1GB";

template <class T>
SpdkCore<T>::SpdkCore(OffloadOptions offloadOptions)
    : state(SpdkState::SPDK_INIT), offloadOptions(offloadOptions), poller(0),
      _spdkThread(0), _loopThread(0), _ready(false) {
    removeConfFile();
    bool conf_file_ok = createConfFile();

    spBdev.reset(new SpdkBdev(true)); // set to true to enable running stats

    if (conf_file_ok == false) {
        if (spdkEnvInit() == false)
            state = SpdkState::SPDK_ERROR;
        else
            state = SpdkState::SPDK_READY;
        spBdev->state = SpdkBdevState::SPDK_BDEV_NOT_FOUND;
    } else
        state = SpdkState::SPDK_READY;
}

template <class T> bool SpdkCore<T>::spdkEnvInit(void) {
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

template <class T> bool SpdkCore<T>::createConfFile(void) {
    if (!bf::exists(DEFAULT_SPDK_CONF_FILE)) {
        if (isNvmeInOptions()) {
            ofstream spdkConf(DEFAULT_SPDK_CONF_FILE, ios::out);
            if (spdkConf) {
                spdkConf << "[Nvme]" << endl
                         << "  TransportID \"trtype:PCIe traddr:"
                         << getNvmeAddress() << "\" " << getNvmeName() << endl;

                spdkConf.close();

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

template <class T> void SpdkCore<T>::removeConfFile(void) {
    if (bf::exists(DEFAULT_SPDK_CONF_FILE)) {
        bf::remove(DEFAULT_SPDK_CONF_FILE);
    }
}

template <class T> void SpdkCore<T>::restoreSignals() {
    ::signal(SIGTERM, SIG_DFL);
    ::signal(SIGINT, SIG_DFL);
    ::signal(SIGSEGV, SIG_DFL);
}

/*
 * Start up Spdk, including SPDK thread, to initialize SPDK environemnt and a
 * Bdev
 */
template <class T> void SpdkCore<T>::startSpdk() {
    _spdkThread = new std::thread(&SpdkCore<T>::_spdkThreadMain, this);
    DAQ_DEBUG("SpdkCore thread started");
    if (_cpuCore) {
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(_cpuCore, &cpuset);

        const int set_result = pthread_setaffinity_np(
            _spdkThread->native_handle(), sizeof(cpu_set_t), &cpuset);
        if (set_result == 0) {
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
template <class T> void SpdkCore<T>::spdkStart(void *arg) {
    SpdkCore<T> *spdkCore = reinterpret_cast<SpdkCore<T> *>(arg);
    SpdkBdev *bdev = spdkCore->spBdev.get();
    SpdkBdevCtx *bdev_c = &bdev->spBdevCtx;

    SpdkConf conf(spdkCore->_bdevName);
    bool rc = bdev->init(conf);
    if (rc == false) {
        DAQ_CRITICAL("Bdev init failed");
        spdkCore->signalReady();
        spdk_app_stop(-1);
        return;
    }

    bdev->setMaxQueued(bdev->getIoCacheSize(), bdev->getBlockSize());
    auto aligned = bdev->getAlignedSize(spdkCore->offloadOptions.allocUnitSize);
    bdev->setBlockNumForLba(aligned / bdev->spBdevCtx.blk_size);

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
        spdk_poller_register(SpdkCore<T>::spdkPollerFunction, spdkCore, 0));

    bdev->setReady();
    spdkCore->signalReady();
    spdkCore->restoreSignals();
}

template <class T> void SpdkCore<T>::_spdkThreadMain(void) {
    struct spdk_app_opts daqdb_opts = {};
    spdk_app_opts_init(&daqdb_opts);
    daqdb_opts.config_file = DEFAULT_SPDK_CONF_FILE.c_str();
    daqdb_opts.name = "daqdb_nvme";

    daqdb_opts.mem_size = 1024;
    daqdb_opts.shm_id = 1;
    daqdb_opts.hugepage_single_segments = 1;
    daqdb_opts.hugedir = SpdkCore<T>::spdkHugepageDirname;

    int rc = spdk_app_start(&daqdb_opts, SpdkCore<T>::spdkStart, this);
    if (rc) {
        DAQ_CRITICAL("Error spdk_app_start[" + std::to_string(rc) + "]");
        return;
    }
    DAQ_DEBUG("spdk_app_start[" + std::to_string(rc) + "]");
    spdk_app_fini();
}

template <class T> int SpdkCore<T>::spdkPollerFunction(void *arg) {
    SpdkCore<T> *spdkCore = reinterpret_cast<SpdkCore<T> *>(arg);
    Poller<T> *poller = spdkCore->poller;

    uint32_t to_qu_cnt = spdkCore->spBdev->canQueue();
    if (to_qu_cnt) {
        poller->dequeue(to_qu_cnt);
        poller->process();
    }

    if (poller->isOffloadRunning() == false) {
        spdk_poller_unregister(&spdkCore->_spdkPoller);
        spdkCore->spBdev->deinit();
        spdk_app_stop(0);
    }

    return 0;
}

template <class T> void SpdkCore<T>::signalReady() {
    std::unique_lock<std::mutex> lk(_syncMutex);
    _ready = true;
    _cv.notify_all();
}

template <class T> bool SpdkCore<T>::waitReady() {
    const std::chrono::milliseconds timeout(10000);
    std::unique_lock<std::mutex> lk(_syncMutex);
    _cv.wait_for(lk, timeout, [this] { return _ready; });
    return _ready;
}

} // namespace DaqDB
