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
#include "SpdkBdev.h"
#include "SpdkConf.h"
#include "SpdkDevice.h"
#include <Logger.h>
#include <RTree.h>

namespace DaqDB {

struct JBODDevice {
    DeviceAddr addr;
    SpdkBdev *bdev;
    uint32_t num;
};

class SpdkJBODBdev : public SpdkDevice {
  public:
    SpdkJBODBdev(bool _statsEnabled = false);
    virtual ~SpdkJBODBdev();

    /**
     * Initialize JBOD devices.
     *
     * @return  if this JBOD devices successfully configured and opened, false
     * otherwise
     */
    virtual bool init(const SpdkConf &conf);
    virtual void deinit();

    /*
     * SpdkDevice virtual interface
     */
    virtual bool read(DeviceTask *task);
    virtual bool write(DeviceTask *task);
    virtual int reschedule(DeviceTask *task);

    virtual void enableStats(bool en);
    virtual size_t getOptimalSize(size_t size) {
        return size ? (((size - 1) / 4096 + 1) * spBdevCtx.io_min_size) : 0;
    }
    virtual size_t getAlignedSize(size_t size) {
        return getOptimalSize(size) + spBdevCtx.blk_size - 1 &
               ~(spBdevCtx.blk_size - 1);
    }
    virtual uint32_t getSizeInBlk(size_t &size) {
        return getOptimalSize(size) / spBdevCtx.blk_size;
    }
    virtual void setReady() { spBdevCtx.state = SPDK_BDEV_READY; }
    virtual bool isOffloadEnabled() {
        return spBdevCtx.state == SPDK_BDEV_READY;
    }
    virtual bool isBdevFound() { return true; }
    virtual void IOQuiesce() {}
    virtual bool isIOQuiescent() { return true; }
    virtual void IOAbort() {}
    virtual uint32_t canQueue();
    virtual SpdkBdevCtx *getBdevCtx() { return &spBdevCtx; }
    virtual uint64_t getBlockOffsetForLba(uint64_t lba) {
        return lba * _blkNumForLba;
    }
    virtual void setBlockNumForLba(uint64_t blk_num_flba) {
        _blkNumForLba = blk_num_flba;
    }
    virtual void setMaxQueued(uint32_t io_cache_size, uint32_t blk_size);
    virtual uint32_t getBlockSize() { return spBdevCtx.blk_size; }
    virtual uint32_t getIoPoolSize() { return spBdevCtx.io_pool_size; }
    virtual uint32_t getIoCacheSize() { return spBdevCtx.io_cache_size; }
    virtual void setRunning(int running) { isRunning = running; }
    virtual bool IsRunning(int running) { return isRunning; }

    static SpdkDeviceClass bdev_class;

    bool statsEnabled;

  private:
    const static uint32_t maxDevices = 64;
    JBODDevice devices[maxDevices];
    uint32_t numDevices = 0;
    uint32_t currDevice = 0;
    std::atomic<int> isRunning;
};

} // namespace DaqDB
