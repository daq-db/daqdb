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

#include <iostream>

#include "spdk/stdinc.h"
#include "spdk/cpuset.h"
#include "spdk/queue.h"
#include "spdk/log.h"
#include "spdk/thread.h"
#include "spdk/event.h"
#include "spdk/ftl.h"
#include "spdk/conf.h"
#include "spdk/env.h"

#include "SpdkBdev.h"
#include <Logger.h>

namespace DaqDB {

const std::string DEFAULT_SPDK_CONF_FILE = "spdk.conf";

SpdkBdev::SpdkBdev() : state(SpdkBdevState::SPDK_BDEV_INIT) {
    spBdevCtx.reset(new SpdkBdevCtx());
}

int SpdkBdev::init(const char *bdev_name) {
    spBdevCtx->bdev_name = bdev_name;
    spBdevCtx->bdev = 0;
    spBdevCtx->bdev_desc = 0;

    spBdevCtx->bdev = spdk_bdev_first();
    if (!spBdevCtx->bdev) {
        DAQ_CRITICAL(std::string("No NVMe devices detected for name[") + spBdevCtx->bdev_name + "]");
        spBdevCtx->state = SPDK_BDEV_NOT_FOUND;
        return false;
    }
    DAQ_DEBUG("NVMe devices detected for name[" + spBdevCtx->bdev_name + "]");

    int rc = spdk_bdev_open(spBdevCtx->bdev, true, NULL, NULL, &spBdevCtx->bdev_desc);
    if (rc) {
        DAQ_CRITICAL("Open BDEV failed with error code[" + std::to_string(rc) + "]");
        spBdevCtx->state = SPDK_BDEV_ERROR;
        return false;
    }

    spBdevCtx->io_channel = spdk_bdev_get_io_channel(spBdevCtx->bdev_desc);
    if (!spBdevCtx->io_channel) {
        DAQ_CRITICAL(std::string("Get io_channel failed bdev[") + spBdevCtx->bdev_name + "]");
        spBdevCtx->state = SPDK_BDEV_ERROR;
        return false;
    }

    spBdevCtx->blk_size = spdk_bdev_get_block_size(spBdevCtx->bdev);
    DAQ_DEBUG("BDEV block size[" + std::to_string(spBdevCtx->blk_size) + "]");

    spBdevCtx->buf_align = spdk_bdev_get_buf_align(spBdevCtx->bdev);
    DAQ_DEBUG("BDEV align[" + std::to_string(spBdevCtx->buf_align) + "]");

    spBdevCtx->blk_num = spdk_bdev_get_num_blocks(spBdevCtx->bdev);
    DAQ_DEBUG("BDEV number of blocks[" + std::to_string(spBdevCtx->blk_num) + "]");

    return true;
}

} // namespace DaqDB
