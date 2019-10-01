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

void daqdb_spdk_start(void *arg) {
//	SpdkBdevCtx *bdev_c = (SpdkBdevCtx *)arg;
//
//    bdev_c->bdev = spdk_bdev_first();
//    if (!bdev_c->bdev) {
//        printf("No NVMe devices detected\n");
//        bdev_c->state = SPDK_BDEV_NOT_FOUND;
//        return;
//    }
//    printf("NVMe devices detected\n");
//
//    int rc = spdk_bdev_open(bdev_c->bdev, true, NULL, NULL, &bdev_c->bdev_desc);
//    if (rc) {
//    	printf("Open BDEV failed with error code[%d]\n", rc);
//    	bdev_c->state = SPDK_BDEV_ERROR;
//        return;
//    }
//
//    bdev_c->io_channel = spdk_bdev_get_io_channel(bdev_c->bdev_desc);
//    if (!bdev_c->io_channel) {
//    	printf("Get io_channel failed\n");
//    	bdev_c->state = SPDK_BDEV_ERROR;
//        return;
//    }
//
//    bdev_c->blk_size = spdk_bdev_get_block_size(bdev_c->bdev);
//    printf("BDEV block size(%u)\n", bdev_c->blk_size);
//
//    bdev_c->buf_align = spdk_bdev_get_buf_align(bdev_c->bdev);
//    printf("BDEV align(%u)\n", bdev_c->buf_align);
//
//    bdev_c->blk_num = spdk_bdev_get_num_blocks(bdev_c->bdev);
//    printf("BDEV number of blocks(%u)\n", bdev_c->blk_num);
//
//    bdev_c->state = SPDK_BDEV_READY;
}

bool SpdkBdev::init() {
//	struct spdk_app_opts daqdb_opts = {};
//	spdk_app_opts_init(&daqdb_opts);
//	daqdb_opts.config_file = DEFAULT_SPDK_CONF_FILE.c_str();
//
//	int rc = spdk_app_start(&daqdb_opts, daqdb_spdk_start, spBdevCtx.get());
//	if ( rc ) {
//		DAQ_DEBUG("Error spdk_app_start[" + std::to_string(rc) + "]");
//		std::cout << "Error spdk_app_start[" << rc << "]" << std::endl;
//		return false;
//	}
//	std::cout << "spdk_app_start[" << rc << "]" << std::endl;

    return true;
}

} // namespace DaqDB
