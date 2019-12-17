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

#include <atomic>
#include <cstdint>
#include <memory>
#include <thread>

#include "spdk/bdev.h"

#include "BdevStats.h"
#include "Rqst.h"
#include "SpdkConf.h"
#include "SpdkDevice.h"
#include <Logger.h>
#include <RTree.h>

namespace DaqDB {

enum class SpdkBdevState : std::uint8_t {
    SPDK_BDEV_INIT = 0,
    SPDK_BDEV_NOT_FOUND,
    SPDK_BDEV_READY,
    SPDK_BDEV_ERROR
};

class FinalizePoller;
class SpdkIoEngine;

class SpdkBdev : public SpdkDevice {
  public:
    SpdkBdev(bool enableStats = false);
    virtual ~SpdkBdev();

    static size_t getCoreNum();

    /**
     * Initialize BDEV device and sets SpdkBdev state.
     *
     * @return true if this BDEV device successfully opened, false otherwise
     */
    virtual bool init(const SpdkConf &conf);
    virtual void deinit();

    bool bdevInit();

    /*
     * Optimal size is 4k times
     */
    inline virtual size_t getOptimalSize(size_t size) {
        return size ? (((size - 1) / 4096 + 1) * spBdevCtx.io_min_size) : 0;
    }
    inline virtual size_t getAlignedSize(size_t size) {
        return getOptimalSize(size) + spBdevCtx.blk_size - 1 &
               ~(spBdevCtx.blk_size - 1);
    }
    inline virtual uint32_t getSizeInBlk(size_t &size) {
        return getOptimalSize(size) / spBdevCtx.blk_size;
    }
    void virtual setReady() { spBdevCtx.state = SPDK_BDEV_READY; }

    virtual bool isOffloadEnabled() {
        return spBdevCtx.state == SPDK_BDEV_READY;
    }
    virtual bool isBdevFound() {
        return state != SpdkBdevState::SPDK_BDEV_NOT_FOUND;
    }
    virtual SpdkBdevCtx *getBdevCtx() { return &spBdevCtx; }
    /*
     * SpdkDevice virtual interface
     */
    virtual bool read(DeviceTask *task);
    virtual bool write(DeviceTask *task);
    bool doRead(DeviceTask *task);
    bool doWrite(DeviceTask *task);
    virtual int reschedule(DeviceTask *task);

    virtual void enableStats(bool en);

    /*
     * Spdk Bdev specifics
     */
    virtual uint32_t getBlockSize() { return spBdevCtx.blk_size; }
    virtual uint32_t getIoPoolSize() { return spBdevCtx.io_pool_size; }
    virtual uint32_t getIoCacheSize() { return spBdevCtx.io_cache_size; }

    /*
     * Callback function for a read IO completion.
     */
    static void readComplete(struct spdk_bdev_io *bdev_io, bool success,
                             void *cb_arg);

    /*
     * Callback function for a write IO completion.
     */
    static void writeComplete(struct spdk_bdev_io *bdev_io, bool success,
                              void *cb_arg);

    /*
     * Callback function that SPDK framework will call when an io buffer becomes
     * available Called by SPDK framework when enough of IO buffers become
     * available to complete the IO
     */
    static void readQueueIoWait(void *cb_arg);

    /*
     * Callback function that SPDK framework will call when an io buffer becomes
     * available Called by SPDK framework when enough of IO buffers become
     * available to complete the IO
     */
    static void writeQueueIoWait(void *cb_arg);

    enum class State : std::uint8_t {
        BDEV_IO_INIT = 0,
        BDEV_IO_READY,
        BDEV_IO_ERROR,
        BDEV_IO_QUIESCING,
        BDEV_IO_QUIESCENT,
        BDEV_IO_STOPPED,
        BDEV_IO_ABORTED
    };

    virtual void IOQuiesce();
    virtual bool isIOQuiescent();
    virtual void IOAbort();

    bool stateMachine() {
        switch (_IoState) {
        case SpdkBdev::State::BDEV_IO_INIT:
        case SpdkBdev::State::BDEV_IO_READY:
            break;
        case SpdkBdev::State::BDEV_IO_ERROR:
            deinit();
            break;
        case SpdkBdev::State::BDEV_IO_QUIESCING:
            if (!stats.outstanding_io_cnt) {
                _IoState = SpdkBdev::State::BDEV_IO_QUIESCENT;
            }
            return true;
        case SpdkBdev::State::BDEV_IO_QUIESCENT:
            return true;
        case SpdkBdev::State::BDEV_IO_STOPPED:
            break;
        case SpdkBdev::State::BDEV_IO_ABORTED:
            deinit();
            return true;
        }
        return false;
    }

    std::atomic<SpdkBdevState> state;
    struct spdk_poller *_spdkPoller;
    volatile State _IoState;

  public:
    static SpdkDeviceClass bdev_class;

    BdevStats stats;

    size_t cpuCore;
    size_t cpuCoreFin;
    size_t cpuCoreIoEng;
    static const size_t cpuCoreStart = 40;
    static size_t cpuCoreCounter;

    virtual uint32_t canQueue() {
        return memTracker->IoBytesQueued >= memTracker->IoBytesMaxQueued
                   ? 0
                   : (memTracker->IoBytesMaxQueued -
                      memTracker->IoBytesQueued) /
                         4096;
    }
    virtual uint64_t getBlockOffsetForLba(uint64_t lba) {
        return lba * _blkNumForLba;
    }
    virtual void setBlockNumForLba(uint64_t blk_num_flba) {
        _blkNumForLba = blk_num_flba;
    }
    virtual void setMaxQueued(uint32_t io_cache_size, uint32_t blk_size);
    virtual void setRunning(int running) { isRunning = running; }
    virtual bool IsRunning(int running) { return isRunning; }

    SpdkIoEngine *ioEngine;
    std::thread *ioEngineThread;
    void ioEngineThreadMain(void);
    static int ioEngineIoFunction(void *arg);

    FinalizePoller *finalizer;
    std::thread *finalizerThread;
    void finilizerThreadMain(void);

  private:
    std::atomic<int> isRunning;
    bool statsEnabled;
    std::atomic<int> ioEngineInitDone;
};

using BdevTask = DeviceTask;

} // namespace DaqDB
