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

namespace DaqDB {

struct SpdkBdevCtx {
    spdk_bdev *bdev;
    spdk_bdev_desc *bdev_desc;
    spdk_io_channel *io_channel;

    uint32_t blk_size = 0;
    uint32_t buf_align = 0;
    uint64_t blk_num = 0;

    char *bdev_name;
};

enum class SpdkBdevState : std::uint8_t {
    SPDK_BDEV_INIT = 0,
    SPDK_BDEV_NOT_FOUND,
    SPDK_BDEV_READY,
    SPDK_BDEV_ERROR
};

class SpdkBdev {
  public:
    SpdkBdev(void);

    /**
     * Initialize BDEV device and sets SpdkBdev state.
     *
     * @return true if this BDEV device successfully opened, false otherwise
     */
    bool init(void);

    inline size_t getAlignedSize(size_t size) {
        return size + spBdevCtx->blk_size - 1 & ~(spBdevCtx->blk_size - 1);
    }
    inline uint32_t getSizeInBlk(size_t &size) {
        return size / spBdevCtx->blk_size;
    }

    std::atomic<SpdkBdevState> state;
    std::unique_ptr<SpdkBdevCtx> spBdevCtx;
};

} // namespace DaqDB
