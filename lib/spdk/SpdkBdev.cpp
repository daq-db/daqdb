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

extern "C" {
#include "spdk/stdinc.h"
#include "spdk/cpuset.h"
#include "spdk/queue.h"
#include "spdk/log.h"
#include "spdk/thread.h"
#include "spdk/event.h"
#include "spdk/ftl.h"
}

#include "SpdkBdev.h"
#include <Logger.h>

extern "C" {
int spdk_ftl_dev_init(const struct spdk_ftl_dev_init_opts *opts, spdk_ftl_init_fn cb, void *cb_arg) { return 0; }
uint64_t spdk_notify_send(const char *type, const char *ctx) { return 0; }
void spdk_ftl_conf_init_defaults(struct spdk_ftl_conf *conf) {}
struct spdk_notify_type *spdk_notify_type_register(const char *type) { return 0; }
int spdk_ftl_flush(struct spdk_ftl_dev *dev, spdk_ftl_fn cb_fn, void *cb_arg) { return 0; }
int spdk_ftl_module_fini(spdk_ftl_fn cb, void *cb_arg) { return 0; }
int spdk_ftl_read(struct spdk_ftl_dev *dev, struct spdk_io_channel *ch, uint64_t lba,
        size_t lba_cnt,
        struct iovec *iov, size_t iov_cnt, spdk_ftl_fn cb_fn, void *cb_arg) { return 0; }
int spdk_ftl_write(struct spdk_ftl_dev *dev, struct spdk_io_channel *ch, uint64_t lba,
        size_t lba_cnt,
        struct iovec *iov, size_t iov_cnt, spdk_ftl_fn cb_fn, void *cb_arg) { return 0; }
void spdk_ftl_dev_get_attrs(const struct spdk_ftl_dev *dev, struct spdk_ftl_attrs *attr) {}
int spdk_ftl_module_init(const struct ftl_module_init_opts *opts, spdk_ftl_fn cb, void *cb_arg) { return 0; }
int spdk_ftl_dev_free(struct spdk_ftl_dev *dev, spdk_ftl_init_fn cb, void *cb_arg) { return 0; }
}

namespace DaqDB {

SpdkBdev::SpdkBdev() : state(SpdkBdevState::SPDK_BDEV_INIT) {
    spBdevCtx.reset(new SpdkBdevCtx());
}

extern "C" void daqdb_spdk_start(void *arg) {
	SpdkBdevCtx *bdev_c = (SpdkBdevCtx *)arg;

    bdev_c->bdev = spdk_bdev_first();
    if (!bdev_c->bdev) {
        printf("No NVMe devices detected\n");
        bdev_c->state = SPDK_BDEV_NOT_FOUND;
        return;
    }

    int rc = spdk_bdev_open(bdev_c->bdev, true, NULL, NULL, &bdev_c->bdev_desc);
    if (rc) {
    	printf("Open BDEV failed with error code[%d]\n", rc);
    	bdev_c->state = SPDK_BDEV_ERROR;
        return;
    }

    bdev_c->io_channel = spdk_bdev_get_io_channel(bdev_c->bdev_desc);
    if (!bdev_c->io_channel) {
    	printf("Get io_channel failed\n");
    	bdev_c->state = SPDK_BDEV_ERROR;
        return;
    }

    bdev_c->blk_size = spdk_bdev_get_block_size(bdev_c->bdev);
    printf("BDEV block size(%u)\n", bdev_c->blk_size);

    bdev_c->buf_align = spdk_bdev_get_buf_align(bdev_c->bdev);
    printf("BDEV align(%u)", bdev_c->buf_align);

    bdev_c->blk_num = spdk_bdev_get_num_blocks(bdev_c->bdev);
    printf("BDEV number of blocks(%ull) " + bdev_c->blk_num);

    bdev_c->state = SPDK_BDEV_READY;
}

bool SpdkBdev::init() {
	struct spdk_app_opts daqdb_opts = {};
	int rc = spdk_app_start(&daqdb_opts, daqdb_spdk_start, spBdevCtx.get());
	if ( rc ) {
		DAQ_DEBUG("Error apdk_app_start[" + std::to_string(rc) + "]");
		return false;
	}

	sleep(2);
    state = SpdkBdevState::SPDK_BDEV_READY;

    return true;
}

} // namespace DaqDB
