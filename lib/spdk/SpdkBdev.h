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

#include "spdk/bdev.h"

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

extern "C" enum CSpdkBdevState {
    SPDK_BDEV_INIT = 0,
    SPDK_BDEV_NOT_FOUND,
    SPDK_BDEV_READY,
    SPDK_BDEV_ERROR
};

extern "C" struct SpdkBdevCtx {
    spdk_bdev *bdev;
    spdk_bdev_desc *bdev_desc;
    spdk_io_channel *io_channel;
    char *buff;
    const char *bdev_name;
    struct spdk_bdev_io_wait_entry bdev_io_wait;
    uint32_t blk_size = 0;
    uint32_t data_blk_size = 0;
    uint32_t buf_align = 0;
    uint64_t blk_num = 0;
    uint32_t io_pool_size = 0;
    uint32_t io_cache_size = 0;
    CSpdkBdevState state;
};

struct BdevStats {
    uint64_t write_compl_cnt;
    uint64_t write_err_cnt;
    uint64_t read_compl_cnt;
    uint64_t read_err_cnt;
    bool periodic = true;
    bool enable;
    uint64_t quant_per = (1 << 17);
    uint64_t outstanding_io_cnt;

    BdevStats(bool enab = false)
        : write_compl_cnt(0), write_err_cnt(0), read_compl_cnt(0),
          read_err_cnt(0), enable(enab), outstanding_io_cnt(0) {}
    std::ostringstream &formatWriteBuf(std::ostringstream &buf);
    std::ostringstream &formatReadBuf(std::ostringstream &buf);
    void printWritePer(std::ostream &os);
    void printReadPer(std::ostream &os);
    void enableStats(bool en);
};

class SpdkBdev : public SpdkDevice {
  public:
    SpdkBdev(bool enableStats = false);
    ~SpdkBdev() = default;

    /**
     * Initialize BDEV device and sets SpdkBdev state.
     *
     * @return true if this BDEV device successfully opened, false otherwise
     */
    virtual bool init(const SpdkConf &conf);
    virtual void deinit();

    inline virtual size_t getAlignedSize(size_t size) {
        return size + spBdevCtx.blk_size - 1 & ~(spBdevCtx.blk_size - 1);
    }
    inline virtual uint32_t getSizeInBlk(size_t &size) {
        return size / spBdevCtx.blk_size;
    }
    void virtual setReady() { spBdevCtx.state = SPDK_BDEV_READY; }

    /*
     * SpdkDevice virtual interface
     */
    virtual int read(DeviceTask *task);
    virtual int write(DeviceTask *task);
    virtual int reschedule(DeviceTask *task);

    virtual void enableStats(bool en);

    /*
     * Spdk Bdev specifics
     */
    inline uint32_t getBlockSize() { return spBdevCtx.blk_size; }
    inline uint32_t getIoPoolSize() { return spBdevCtx.io_pool_size; }
    inline uint32_t getIoCacheSize() { return spBdevCtx.io_cache_size; }

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

    void IOQuiesce();
    bool isIOQuiescent();
    void IOAbort();

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

    SpdkBdevCtx spBdevCtx;
    uint64_t _blkNumForLba = 0;
    BdevStats stats;

    uint64_t IoBytesQueued;
    uint64_t IoBytesMaxQueued;

    uint32_t canQueue() {
        return IoBytesQueued >= IoBytesMaxQueued
                   ? 0
                   : (IoBytesMaxQueued - IoBytesQueued) / 4096;
    }
    uint64_t getBlockOffsetForLba(uint64_t lba) { return lba * _blkNumForLba; }
    void setBlockNumForLba(uint64_t blk_num_flba) {
        _blkNumForLba = blk_num_flba;
    }
    void setMaxQueued(uint32_t io_cache_size, uint32_t blk_size);
};

using BdevTask = DeviceTask;

} // namespace DaqDB
