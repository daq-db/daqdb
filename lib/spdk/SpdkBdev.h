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
