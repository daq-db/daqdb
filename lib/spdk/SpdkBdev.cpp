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

#include <stdio.h>
#include <time.h>

#include <iostream>
#include <sstream>
#include <string>

#include <boost/asio.hpp>
#include <boost/filesystem.hpp>

#include "spdk/stdinc.h"
#include "spdk/cpuset.h"
#include "spdk/queue.h"
#include "spdk/log.h"
#include "spdk/thread.h"
#include "spdk/event.h"
#include "spdk/ftl.h"
#include "spdk/conf.h"
#include "spdk/env.h"

#include "Rqst.h"
#include "SpdkBdev.h"
#include <FinalizePoller.h>
#include <SpdkIoEngine.h>
#include <daqdb/Status.h>

namespace DaqDB {

//#define TEST_RAW_IOPS

const char *SpdkBdev::lbaMgmtFileprefix = "/mnt/pmem/bdev_free_lba_list_";

const unsigned int poolFreelistSize = 1ULL * 1024 * 1024 * 1024;
const char *poolLayout = "queue";
#define CREATE_MODE_RW (S_IWUSR | S_IRUSR)

/*
 * SpdkBdev
 */
const std::string DEFAULT_SPDK_CONF_FILE = "spdk.conf";

SpdkDeviceClass SpdkBdev::bdev_class = SpdkDeviceClass::BDEV;
size_t SpdkBdev::cpuCoreCounter = SpdkBdev::cpuCoreStart;

SpdkBdev::SpdkBdev(bool enableStats)
    : state(SpdkBdevState::SPDK_BDEV_INIT), _spdkPoller(0), confBdevNum(-1),
      cpuCore(SpdkBdev::getCoreNum()), cpuCoreFin(cpuCore + 1),
      cpuCoreIoEng(cpuCoreFin + 1), finalizer(0), finalizerThread(0),
      ioEngine(0), ioEngineThread(0), isRunning(0), statsEnabled(enableStats),
      ioEngineInitDone(0), maxIoBufs(0), ioBufsInUse(0), maxCacheIoBufs(0) {}

SpdkBdev::~SpdkBdev() {
    if (finalizerThread != nullptr)
        finalizerThread->join();
    if (finalizer)
        delete finalizer;

    if (ioEngineThread != nullptr)
        ioEngineThread->join();
    if (ioEngine)
        delete ioEngine;
}

size_t SpdkBdev::getCoreNum() {
    SpdkBdev::cpuCoreCounter += 3;
    return SpdkBdev::cpuCoreCounter;
}

void SpdkBdev::IOQuiesce() { _IoState = SpdkBdev::State::BDEV_IO_QUIESCING; }

bool SpdkBdev::isIOQuiescent() {
    return _IoState == SpdkBdev::State::BDEV_IO_QUIESCENT ||
                   _IoState == SpdkBdev::State::BDEV_IO_ABORTED
               ? true
               : false;
}

void SpdkBdev::IOAbort() { _IoState = SpdkBdev::State::BDEV_IO_ABORTED; }

/*
 * Callback function for a write IO completion.
 */
void SpdkBdev::writeComplete(struct spdk_bdev_io *bdev_io, bool success,
                             void *cb_arg) {
    BdevTask *task = reinterpret_cast<DeviceTask *>(cb_arg);
    SpdkBdev *bdev = reinterpret_cast<SpdkBdev *>(task->bdev);
    spdk_dma_free(task->buff);
    bdev->ioBufsInUse--;

#ifndef TEST_RAW_IOPS
    spdk_bdev_free_io(bdev_io);
#endif

    if (bdev->stats.outstanding_io_cnt)
        bdev->stats.outstanding_io_cnt--;

    bdev->stats.write_compl_cnt++;
    (void)bdev->stateMachine();

    task->result = success;
    if (bdev->statsEnabled == true && success == true)
        bdev->stats.printWritePer(std::cout, bdev->spBdevCtx.bdev_addr);
    bdev->finalizer->enqueue(task);
}

/*
 * Callback function for a read IO completion.
 */
void SpdkBdev::readComplete(struct spdk_bdev_io *bdev_io, bool success,
                            void *cb_arg) {
    BdevTask *task = reinterpret_cast<DeviceTask *>(cb_arg);
    SpdkBdev *bdev = reinterpret_cast<SpdkBdev *>(task->bdev);

#ifndef TEST_RAW_IOPS
    spdk_bdev_free_io(bdev_io);
#endif

    if (bdev->stats.outstanding_io_cnt)
        bdev->stats.outstanding_io_cnt--;

    bdev->stats.read_compl_cnt++;
    (void)bdev->stateMachine();

    task->result = success;
    if (bdev->statsEnabled == true && success == true)
        bdev->stats.printReadPer(std::cout, bdev->spBdevCtx.bdev_addr);
    bdev->finalizer->enqueue(task);
}

/*
 * Callback function that SPDK framework will call when an io buffer becomes
 * available
 */
void SpdkBdev::readQueueIoWait(void *cb_arg) {
    BdevTask *task = reinterpret_cast<DeviceTask *>(cb_arg);
    SpdkBdev *bdev = reinterpret_cast<SpdkBdev *>(task->bdev);

    int r_rc = spdk_bdev_read_blocks(
        bdev->spBdevCtx.bdev_desc, bdev->spBdevCtx.io_channel, task->buff,
        task->blockSize * task->bdevAddr->lba, task->blockSize,
        SpdkBdev::readComplete, task);

    /* If a read IO still fails due to shortage of io buffers, queue it up for
     * later execution */
    if (r_rc) {
        r_rc = spdk_bdev_queue_io_wait(bdev->spBdevCtx.bdev,
                                       bdev->spBdevCtx.io_channel,
                                       &task->bdev_io_wait);
        if (r_rc) {
            DAQ_CRITICAL("Spdk queue_io_wait error [" + std::to_string(r_rc) +
                         "] for spdk bdev");
            bdev->deinit();
            spdk_app_stop(-1);
        }
    } else {
        bdev->stats.read_err_cnt--;
    }
}

/*
 * Callback function that SPDK framework will call when an io buffer becomes
 * available
 */
void SpdkBdev::writeQueueIoWait(void *cb_arg) {
    BdevTask *task = reinterpret_cast<DeviceTask *>(cb_arg);
    SpdkBdev *bdev = reinterpret_cast<SpdkBdev *>(task->bdev);

    int w_rc = spdk_bdev_write_blocks(
        bdev->spBdevCtx.bdev_desc, bdev->spBdevCtx.io_channel, task->buff,
        task->blockSize * task->freeLba, task->blockSize,
        SpdkBdev::writeComplete, task);

    /* If a write IO still fails due to shortage of io buffers, queue it up for
     * later execution */
    if (w_rc) {
        w_rc = spdk_bdev_queue_io_wait(bdev->spBdevCtx.bdev,
                                       bdev->spBdevCtx.io_channel,
                                       &task->bdev_io_wait);
        if (w_rc) {
            DAQ_CRITICAL("Spdk queue_io_wait error [" + std::to_string(w_rc) +
                         "] for spdk bdev");
            bdev->deinit();
            spdk_app_stop(-1);
        }
    } else {
        bdev->stats.write_err_cnt--;
    }
}

bool SpdkBdev::read(DeviceTask *task) {
    if (ioEngine && task->routing == true)
        return ioEngine->enqueue(task);
    return doRead(task);
}

bool SpdkBdev::doRead(DeviceTask *task) {
    SpdkBdev *bdev = reinterpret_cast<SpdkBdev *>(task->bdev);
    if (stateMachine() == true) {
        spdk_dma_free(task->buff);
        bdev->ioBufsInUse--;
        return false;
    }

    size_t algnSize = bdev->getAlignedSize(task->rqst->valueSize);
    auto blkSize = bdev->getSizeInBlk(algnSize);
    task->blockSize = blkSize;

    char *buff = reinterpret_cast<char *>(
        spdk_dma_zmalloc(task->size, bdev->spBdevCtx.buf_align, NULL));
    bdev->ioBufsInUse++;
    task->buff = buff;

#ifdef TEST_RAW_IOPS
    int r_rc = 0;
    SpdkBdev::readComplete(reinterpret_cast<struct spdk_bdev_io *>(task->buff),
                           true, task);
#else
    int r_rc = spdk_bdev_read_blocks(
        bdev->spBdevCtx.bdev_desc, bdev->spBdevCtx.io_channel, task->buff,
        task->blockSize * task->bdevAddr->lba, task->blockSize,
        SpdkBdev::readComplete, task);
#endif
    if (r_rc) {
        stats.read_err_cnt++;
        /*
         * -ENOMEM condition indicates a shortage of IO buffers.
         *  Schedule this IO for later execution by providing a callback
         * function, when sufficient IO buffers are available
         */
        if (r_rc == -ENOMEM) {
            r_rc = SpdkBdev::reschedule(task);
            if (r_rc) {
                DAQ_CRITICAL("Spdk queue_io_wait error [" +
                             std::to_string(r_rc) + "] for spdk bdev");
                bdev->deinit();
                spdk_app_stop(-1);
            }
        } else {
            DAQ_CRITICAL("Spdk read error [" + std::to_string(r_rc) +
                         "] for spdk bdev");
            bdev->deinit();
            spdk_app_stop(-1);
        }
    }

    return !r_rc ? true : false;
}

bool SpdkBdev::write(DeviceTask *task) {
    if (ioEngine && task->routing == true)
        return ioEngine->enqueue(task);
    return doWrite(task);
}

bool SpdkBdev::doWrite(DeviceTask *task) {
    SpdkBdev *bdev = reinterpret_cast<SpdkBdev *>(task->bdev);
    if (stateMachine() == true) {
        spdk_dma_free(task->buff);
        bdev->ioBufsInUse--;
        return false;
    }

    auto valSize = task->rqst->valueSize;
    auto valSizeAlign = bdev->getAlignedSize(valSize);
    if (task->rqst->loc == LOCATIONS::PMEM)
        task->freeLba = getFreeLba(valSizeAlign);
    auto buff = reinterpret_cast<char *>(
        spdk_dma_zmalloc(valSizeAlign, bdev->spBdevCtx.buf_align, NULL));
    bdev->ioBufsInUse++;

    memcpy(buff, task->rqst->value, valSize);
    task->buff = buff;

#ifdef TEST_RAW_IOPS
    int w_rc = 0;
    SpdkBdev::writeComplete(reinterpret_cast<struct spdk_bdev_io *>(task->buff),
                            true, task);
#else
    int w_rc = spdk_bdev_write_blocks(
        bdev->spBdevCtx.bdev_desc, bdev->spBdevCtx.io_channel, task->buff,
        task->blockSize * task->freeLba, task->blockSize,
        SpdkBdev::writeComplete, task);
#endif

    if (w_rc) {
        stats.write_err_cnt++;
        /*
         * -ENOMEM condition indicates a shortage of IO buffers.
         *  Schedule this IO for later execution by providing a callback
         * function, when sufficient IO buffers are available
         */
        if (w_rc == -ENOMEM) {
            w_rc = SpdkBdev::reschedule(task);
            if (w_rc) {
                DAQ_CRITICAL("Spdk queue_io_wait error [" +
                             std::to_string(w_rc) + "] for spdk bdev");
                bdev->deinit();
                spdk_app_stop(-1);
            }
        } else {
            DAQ_CRITICAL("Spdk write error [" + std::to_string(w_rc) +
                         "] for spdk bdev");
            bdev->deinit();
            spdk_app_stop(-1);
        }
    }

    return !w_rc ? true : false;
}

bool SpdkBdev::remove(DeviceTask *task) {
    if (ioEngine && task->routing == true)
        return ioEngine->enqueue(task);
    return doRemove(task);
}

bool SpdkBdev::doRemove(DeviceTask *task) {
    if (stateMachine() == true) {
        return false;
    }

    auto valSize = task->rqst->valueSize;
    auto valSizeAlign = getAlignedSize(valSize);

    putFreeLba(task->bdevAddr, valSizeAlign);
    task->result = true;
    finalizer->enqueue(task);

    return true;
}

int SpdkBdev::reschedule(DeviceTask *task) {
    SpdkBdev *bdev = reinterpret_cast<SpdkBdev *>(task->bdev);

    bdev->stats.outstanding_io_cnt++;
    task->bdev_io_wait.bdev = spBdevCtx.bdev;
    task->bdev_io_wait.cb_fn = SpdkBdev::writeQueueIoWait;
    task->bdev_io_wait.cb_arg = task;

    return spdk_bdev_queue_io_wait(
        bdev->spBdevCtx.bdev, bdev->spBdevCtx.io_channel, &task->bdev_io_wait);
}

void SpdkBdev::deinit() {
    isRunning = 4;
    while (isRunning == 4) {
    }
    spdk_put_io_channel(spBdevCtx.io_channel);
    spdk_bdev_close(spBdevCtx.bdev_desc);
}

struct spdk_bdev *SpdkBdev::prevBdev = 0;

int64_t SpdkBdev::getFreeLba(size_t ioSize) {
    return lbaAllocator->getLba(ioSize);
}

void SpdkBdev::putFreeLba(const DeviceAddr *devAddr, size_t ioSize) {
    lbaAllocator->putLba(devAddr->lba, ioSize);
}

void SpdkBdev::initFreeList() {
    std::string fileName =
        std::string(SpdkBdev::lbaMgmtFileprefix) + spBdevCtx.bdev_name + ".pm";
    lbaAllocator =
        new OffloadLbaAlloc(true, fileName, spBdevCtx.blk_num, blkNumForLba);
}

bool SpdkBdev::bdevInit() {
    if (confBdevNum == -1) { // single Bdev
        spBdevCtx.bdev = spdk_bdev_first();
    } else { // JBOD or RAID0
        if (!confBdevNum) {
            spBdevCtx.bdev = spdk_bdev_first_leaf();
            SpdkBdev::prevBdev = spBdevCtx.bdev;
        } else if (confBdevNum > 0) {
            spBdevCtx.bdev = spdk_bdev_next_leaf(SpdkBdev::prevBdev);
            SpdkBdev::prevBdev = spBdevCtx.bdev;
        } else {
            DAQ_CRITICAL("Get leaf BDEV failed");
            spBdevCtx.state = SPDK_BDEV_NOT_FOUND;
            return false;
        }
    }

    if (!spBdevCtx.bdev) {
        DAQ_CRITICAL(std::string("No NVMe devices detected for name[") +
                     spBdevCtx.bdev_name + "]");
        spBdevCtx.state = SPDK_BDEV_NOT_FOUND;
        return false;
    }
    DAQ_DEBUG("NVMe devices detected for name[" + spBdevCtx->bdev_name + "]");

    spdk_bdev_opts bdev_opts;
    int rc = spdk_bdev_open(spBdevCtx.bdev, true, 0, 0, &spBdevCtx.bdev_desc);
    if (rc) {
        DAQ_CRITICAL("Open BDEV failed with error code[" + std::to_string(rc) + "]");
        spBdevCtx.state = SPDK_BDEV_ERROR;
        return false;
    }

    spdk_bdev_get_opts(&bdev_opts);
    DAQ_DEBUG("bdev.bdev_io_pool_size[" + bdev_opts.bdev_io_pool_size + "]" +
              " bdev.bdev.io_cache_size[" + bdev_opts.bdev_io_cache_size + "]");

    spBdevCtx.io_pool_size = bdev_opts.bdev_io_pool_size;
    maxIoBufs = spBdevCtx.io_pool_size;

    spBdevCtx.io_cache_size = bdev_opts.bdev_io_cache_size;
    maxCacheIoBufs = spBdevCtx.io_cache_size;

    spBdevCtx.io_channel = spdk_bdev_get_io_channel(spBdevCtx.bdev_desc);
    if (!spBdevCtx.io_channel) {
        DAQ_CRITICAL(std::string("Get io_channel failed bdev[") +
                     spBdevCtx.bdev_name + "]");
        spBdevCtx.state = SPDK_BDEV_ERROR;
        return false;
    }

    spBdevCtx.blk_size = spdk_bdev_get_block_size(spBdevCtx.bdev);
    DAQ_DEBUG("BDEV block size[" + std::to_string(spBdevCtx.blk_size) + "]");

    spBdevCtx.data_blk_size = spdk_bdev_get_data_block_size(spBdevCtx.bdev);
    DAQ_DEBUG("BDEV data block size[" + spBdevCtx.data_blk_size + "]");

    spBdevCtx.buf_align = spdk_bdev_get_buf_align(spBdevCtx.bdev);
    DAQ_DEBUG("BDEV align[" + std::to_string(spBdevCtx.buf_align) + "]");

    spBdevCtx.blk_num = spdk_bdev_get_num_blocks(spBdevCtx.bdev);
    DAQ_DEBUG("BDEV number of blocks[" + std::to_string(spBdevCtx.blk_num) +
              "]");

    ioEngineInitDone = 1;
    return true;
}

bool SpdkBdev::init(const SpdkConf &conf) {
    confBdevNum = conf.getBdevNum();
    strcpy(spBdevCtx.bdev_name, conf.getBdevNvmeName().c_str());
    strcpy(spBdevCtx.bdev_addr, conf.getBdevNvmeAddr().c_str());
    spBdevCtx.bdev = 0;
    spBdevCtx.bdev_desc = 0;
    spBdevCtx.pci_addr = conf.getBdevSpdkPciAddr();

    setRunning(3);

    /*
     * Set up finalizer
     */
    finalizer = new FinalizePoller();
    finalizerThread = new std::thread(&SpdkBdev::finilizerThreadMain, this);
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpuCoreFin, &cpuset);

    int set_result = pthread_setaffinity_np(finalizerThread->native_handle(),
                                            sizeof(cpu_set_t), &cpuset);
    if (!set_result) {
        DAQ_DEBUG("SpdkCore thread affinity set on CPU core [" +
                  std::to_string(cpuCoreFin) + "]");
    } else {
        DAQ_DEBUG("Cannot set affinity on CPU core [" +
                  std::to_string(cpuCoreFin) + "] for Finalizer");
    }

    /*
     * Set up ioEngine
     */
    ioEngine = new SpdkIoEngine();
    ioEngineThread = new std::thread(&SpdkBdev::ioEngineThreadMain, this);
    cpu_set_t cpuset1;
    CPU_ZERO(&cpuset1);
    CPU_SET(cpuCoreIoEng, &cpuset1);

    set_result = pthread_setaffinity_np(ioEngineThread->native_handle(),
                                        sizeof(cpu_set_t), &cpuset1);
    if (!set_result) {
        DAQ_DEBUG("SpdkCore thread affinity set on CPU core [" +
                  std::to_string(cpuCoreIoEng) + "]");
    } else {
        DAQ_DEBUG("Cannot set affinity on CPU core [" +
                  std::to_string(cpuCoreIoEng) + "] for IoEngine");
    }

    while (!ioEngineInitDone) {
    }
    setRunning(1);

    return true;
}

void SpdkBdev::finilizerThreadMain() {
    std::string finThreadName = std::string(spBdevCtx.bdev_name) + "_finalizer";
    pthread_setname_np(pthread_self(), finThreadName.c_str());
    while (isRunning == 3) {
    }
    while (isRunning) {
        finalizer->dequeue();
        finalizer->process();
    }
}

int SpdkBdev::ioEngineIoFunction(void *arg) {
    SpdkBdev *bdev = reinterpret_cast<SpdkBdev *>(arg);
    if (bdev->isRunning) {
        uint32_t can_queue_cnt = bdev->canQueue();
        if (can_queue_cnt) {
            bdev->ioEngine->dequeue(can_queue_cnt);
            bdev->ioEngine->process();
        }
    }

    return 0;
}

void SpdkBdev::ioEngineThreadMain() {
    std::string finThreadName = std::string(spBdevCtx.bdev_name) + "_io";
    pthread_setname_np(pthread_self(), finThreadName.c_str());

    struct spdk_thread *spdk_th = spdk_thread_create(spBdevCtx.bdev_name, 0);
    if (!spdk_th) {
        DAQ_CRITICAL(
            "Spdk spdk_thread_create() can't create context on pthread");
        deinit();
        spdk_app_stop(-1);
        return;
    }
    spdk_set_thread(spdk_th);

    bool ret = bdevInit();
    if (ret == false) {
        DAQ_CRITICAL("Bdev init failed");
        return;
    }

    /*
     * Wait for ready on
     */
    while (isRunning == 3) {
    }

    struct spdk_poller *spdk_io_poller =
        spdk_poller_register(SpdkBdev::ioEngineIoFunction, this, 0);
    if (!spdk_io_poller) {
        DAQ_CRITICAL("Spdk poller can't be created");
        spdk_thread_exit(spdk_th);
        deinit();
        spdk_app_stop(-1);
        return;
    }

    /*
     * Ensure only IOs and events for this SPDK thread are processed
     * by this Bdev
     */
    for (;;) {
        if (isRunning == 4)
            break;
        int ret = spdk_thread_poll(spdk_th, 0, 0);
        if (ret < 0)
            break;
    }
    isRunning = 5;

    spdk_poller_unregister(&spdk_io_poller);
}

void SpdkBdev::setMaxQueued(uint32_t io_cache_size, uint32_t blk_size) {}

void SpdkBdev::enableStats(bool en) { statsEnabled = en; }

} // namespace DaqDB
