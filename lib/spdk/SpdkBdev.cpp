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

#include "SpdkBdev.h"
#include <Logger.h>

namespace DaqDB {

SpdkBdev::SpdkBdev() : state(SpdkBdevState::SPDK_BDEV_INIT) {
    spBdevCtx.reset(new SpdkBdevCtx());
}

bool SpdkBdev::init() {
    spBdevCtx->bdev = spdk_bdev_first();
    if (!spBdevCtx->bdev) {
        DAQ_DEBUG("No NVMe devices detected");
        state = SpdkBdevState::SPDK_BDEV_NOT_FOUND;
        return false;
    }

    auto rc = spdk_bdev_open(spBdevCtx->bdev, true, NULL, NULL,
                             &spBdevCtx->bdev_desc);
    if (rc) {
        DAQ_DEBUG("Open BDEV failed with error code [" + std::to_string(rc) +
                  "]");
        state = SpdkBdevState::SPDK_BDEV_ERROR;
        return false;
    }

    spBdevCtx->io_channel = spdk_bdev_get_io_channel(spBdevCtx->bdev_desc);
    if (!spBdevCtx->io_channel) {
        DAQ_DEBUG("Get io_channel failed");
        state = SpdkBdevState::SPDK_BDEV_ERROR;
        return false;
    }

    spBdevCtx->blk_size = spdk_bdev_get_block_size(spBdevCtx->bdev);
    DAQ_DEBUG("BDEV block size: " + std::to_string(spBdevCtx->blk_size));
    spBdevCtx->buf_align = spdk_bdev_get_buf_align(spBdevCtx->bdev);
    DAQ_DEBUG("BDEV align: " + std::to_string(spBdevCtx->buf_align));
    spBdevCtx->blk_num = spdk_bdev_get_num_blocks(spBdevCtx->bdev);
    DAQ_DEBUG("BDEV number of blocks: " + std::to_string(spBdevCtx->blk_num));

    state = SpdkBdevState::SPDK_BDEV_READY;

    return true;
}

} // namespace DaqDB
