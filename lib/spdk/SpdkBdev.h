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
#include "SpdkDevice.h"

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

class SpdkBdev : public SpdkDevice<OffloadRqst> {
  public:
    SpdkBdev(void);
    ~SpdkBdev() = default;

    /**
     * Initialize BDEV device and sets SpdkBdev state.
     *
     * @return true if this BDEV device successfully opened, false otherwise
     */
    bool init(const char *bdev_name);
    void deinit();

    inline size_t getAlignedSize(size_t size) {
        return size + spBdevCtx->blk_size - 1 & ~(spBdevCtx->blk_size - 1);
    }
    inline uint32_t getSizeInBlk(size_t &size) {
        return size / spBdevCtx->blk_size;
    }
    void setReady() {
        spBdevCtx->state = SPDK_BDEV_READY;
    }

    /*
     * SpdkDevice virtual interface
     */
    virtual int read(OffloadRqst *rqst);
    virtual int write(OffloadRqst *rqst);

    /*
     * Spdk Bdev specifics
     */
    inline uint32_t getBlockSize() { return spBdevCtx->blk_size; }
    inline uint32_t getIoPoolSize() { return spBdevCtx->io_pool_size; }
    inline uint32_t getIoCacheSize() { return spBdevCtx->io_cache_size; }

    std::atomic<SpdkBdevState> state;
    std::unique_ptr<SpdkBdevCtx> spBdevCtx;
};

} // namespace DaqDB
