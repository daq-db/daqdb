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

#include <stdio.h>
#include <time.h>

#include <iostream>
#include <sstream>
#include <string>

#include "spdk/stdinc.h"
#include "spdk/cpuset.h"
#include "spdk/queue.h"
#include "spdk/log.h"
#include "spdk/thread.h"
#include "spdk/event.h"
#include "spdk/ftl.h"
#include "spdk/conf.h"
#include "spdk/env.h"

#include "Rqst.h"
#include "SpdkBdev.h"
#include <daqdb/Status.h>

namespace DaqDB {

//#define TEST_RAW_IOPS

/*
 * BdevStats
 */
std::ostringstream &BdevStats::formatWriteBuf(std::ostringstream &buf) {
    buf << "write_compl_cnt[" << write_compl_cnt << "] write_err_cnt["
        << write_err_cnt << "] outs_io_cnt[" << outstanding_io_cnt << "]";
    return buf;
}

std::ostringstream &BdevStats::formatReadBuf(std::ostringstream &buf) {
    buf << "read_compl_cnt[" << read_compl_cnt << "] read_err_cnt["
        << read_err_cnt << "] outs_io_cnt[" << outstanding_io_cnt << "]";
    return buf;
}

void BdevStats::printWritePer(std::ostream &os) {
    if (enable == true && !((write_compl_cnt++) % quant_per)) {
        std::ostringstream buf;
        char time_buf[128];
        time_t now = time(0);
        strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S.000",
                 localtime(&now));
        os << formatWriteBuf(buf).str() << " " << time_buf << std::endl;
    }
}

void BdevStats::printReadPer(std::ostream &os) {
    if (enable == true && !((read_compl_cnt++) % quant_per)) {
        std::ostringstream buf;
        char time_buf[128];
        time_t now = time(0);
        strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S.000",
                 localtime(&now));
        os << formatReadBuf(buf).str() << " " << time_buf << std::endl;
    }
}

void BdevStats::enableStats(bool en) { enable = en; }
/*
 * SpdkBdev
 */
const std::string DEFAULT_SPDK_CONF_FILE = "spdk.conf";

SpdkDeviceClass SpdkBdev::bdev_class = SpdkDeviceClass::BDEV;

SpdkBdev::SpdkBdev(bool enableStats)
    : state(SpdkBdevState::SPDK_BDEV_INIT), stats(enableStats),
      IoBytesQueued(0), IoBytesMaxQueued(0) {}

void SpdkBdev::IOQuiesce() { _IoState = SpdkBdev::State::BDEV_IO_QUIESCING; }

bool SpdkBdev::isIOQuiescent() {
    return _IoState == SpdkBdev::State::BDEV_IO_QUIESCENT ||
                   _IoState == SpdkBdev::State::BDEV_IO_ABORTED
               ? true
               : false;
}

void SpdkBdev::IOAbort() { _IoState = SpdkBdev::State::BDEV_IO_ABORTED; }

/*
 * Callback function for a write IO completion.
 */
void SpdkBdev::writeComplete(struct spdk_bdev_io *bdev_io, bool success,
                             void *cb_arg) {
    BdevTask *task = reinterpret_cast<DeviceTask *>(cb_arg);
    SpdkBdev *bdev = reinterpret_cast<SpdkBdev *>(task->bdev);
    spdk_dma_free(task->buff);
    bdev->IoBytesQueued =
        task->size > bdev->IoBytesQueued ? 0 : bdev->IoBytesQueued - task->size;

#ifndef TEST_RAW_IOPS
    spdk_bdev_free_io(bdev_io);
#endif

    if (bdev->stats.outstanding_io_cnt)
        bdev->stats.outstanding_io_cnt--;

    (void)bdev->stateMachine();

    if (success) {
        bdev->stats.printWritePer(std::cout);
        if (task->updatePmemIOV)
            task->rtree->UpdateValueWrapper(task->key, task->lba,
                                            sizeof(uint64_t));
        if (task->clb)
            task->clb(nullptr, StatusCode::OK, task->key, task->keySize,
                      task->buff, task->size);
    } else {
        if (task->clb)
            task->clb(nullptr, StatusCode::UNKNOWN_ERROR, task->key,
                      task->keySize, nullptr, 0);
    }

    delete task->rqst;
}

/*
 * Callback function for a read IO completion.
 */
void SpdkBdev::readComplete(struct spdk_bdev_io *bdev_io, bool success,
                            void *cb_arg) {
    BdevTask *task = reinterpret_cast<DeviceTask *>(cb_arg);
    SpdkBdev *bdev = reinterpret_cast<SpdkBdev *>(task->bdev);
    spdk_dma_free(task->buff);
    bdev->IoBytesQueued =
        task->size > bdev->IoBytesQueued ? 0 : bdev->IoBytesQueued - task->size;

#ifndef TEST_RAW_IOPS
    spdk_bdev_free_io(bdev_io);
#endif

    if (bdev->stats.outstanding_io_cnt)
        bdev->stats.outstanding_io_cnt--;

    (void)bdev->stateMachine();

    if (success) {
        bdev->stats.printReadPer(std::cout);
        if (task->clb)
            task->clb(nullptr, StatusCode::OK, task->key, task->keySize,
                      task->buff, task->size);
    } else {
        if (task->clb)
            task->clb(nullptr, StatusCode::UNKNOWN_ERROR, task->key,
                      task->keySize, nullptr, 0);
    }

    delete task->rqst;
}

/*
 * Callback function that SPDK framework will call when an io buffer becomes
 * available
 */
void SpdkBdev::readQueueIoWait(void *cb_arg) {
    BdevTask *task = reinterpret_cast<DeviceTask *>(cb_arg);
    SpdkBdev *bdev = reinterpret_cast<SpdkBdev *>(task->bdev);

    int r_rc = spdk_bdev_read_blocks(
        bdev->spBdevCtx.bdev_desc, bdev->spBdevCtx.io_channel, task->buff,
        bdev->getBlockOffsetForLba(*task->lba), task->blockSize,
        SpdkBdev::readComplete, task);

    /* If a read IO still fails due to shortage of io buffers, queue it up for
     * later execution */
    if (r_rc) {
        r_rc = spdk_bdev_queue_io_wait(bdev->spBdevCtx.bdev,
                                       bdev->spBdevCtx.io_channel,
                                       &task->bdev_io_wait);
        if (r_rc) {
            DAQ_CRITICAL("Spdk queue_io_wait error [" + std::to_string(r_rc) +
                         "] for spdk bdev");
            bdev->deinit();
            spdk_app_stop(-1);
        }
    } else {
        bdev->stats.read_err_cnt--;
        bdev->stats.outstanding_io_cnt++;
    }
}

/*
 * Callback function that SPDK framework will call when an io buffer becomes
 * available
 */
void SpdkBdev::writeQueueIoWait(void *cb_arg) {
    BdevTask *task = reinterpret_cast<DeviceTask *>(cb_arg);
    SpdkBdev *bdev = reinterpret_cast<SpdkBdev *>(task->bdev);

    int w_rc = spdk_bdev_write_blocks(
        bdev->spBdevCtx.bdev_desc, bdev->spBdevCtx.io_channel, task->buff,
        bdev->getBlockOffsetForLba(*task->lba), task->blockSize,
        SpdkBdev::writeComplete, task);

    /* If a write IO still fails due to shortage of io buffers, queue it up for
     * later execution */
    if (w_rc) {
        w_rc = spdk_bdev_queue_io_wait(bdev->spBdevCtx.bdev,
                                       bdev->spBdevCtx.io_channel,
                                       &task->bdev_io_wait);
        if (w_rc) {
            DAQ_CRITICAL("Spdk queue_io_wait error [" + std::to_string(w_rc) +
                         "] for spdk bdev");
            bdev->deinit();
            spdk_app_stop(-1);
        }
    } else {
        bdev->stats.write_err_cnt--;
        bdev->stats.outstanding_io_cnt++;
    }
}

int SpdkBdev::read(DeviceTask *task) {
    SpdkBdev *bdev = reinterpret_cast<SpdkBdev *>(task->bdev);
    if (stateMachine() == true) {
        spdk_dma_free(task->buff);
        return false;
    }

#ifdef TEST_RAW_IOPS
    int r_rc = 0;
    SpdkBdev::readComplete(reinterpret_cast<struct spdk_bdev_io *>(task->buff),
                           true, task);
#else
    int r_rc = spdk_bdev_read_blocks(
        bdev->spBdevCtx.bdev_desc, bdev->spBdevCtx.io_channel, task->buff,
        task->blockSize * (*task->lba), task->blockSize, SpdkBdev::readComplete,
        task);
#endif
    if (r_rc) {
        stats.read_err_cnt++;
        /*
         * -ENOMEM condition indicates a shoortage of IO buffers.
         *  Schedule this IO for later execution by providing a callback
         * function, when sufficient IO buffers are available
         */
        if (r_rc == -ENOMEM) {
            r_rc = SpdkBdev::reschedule(task);
            if (r_rc) {
                DAQ_CRITICAL("Spdk queue_io_wait error [" +
                             std::to_string(r_rc) + "] for spdk bdev");
                bdev->deinit();
                spdk_app_stop(-1);
            }
        } else {
            DAQ_CRITICAL("Spdk read error [" + std::to_string(r_rc) +
                         "] for spdk bdev");
            bdev->deinit();
            spdk_app_stop(-1);
        }
    } else
        stats.outstanding_io_cnt++;

    return !r_rc ? true : false;
}

int SpdkBdev::write(DeviceTask *task) {
    SpdkBdev *bdev = reinterpret_cast<SpdkBdev *>(task->bdev);
    if (stateMachine() == true) {
        spdk_dma_free(task->buff);
        return false;
    }

#ifdef TEST_RAW_IOPS
    int w_rc = 0;
    SpdkBdev::writeComplete(reinterpret_cast<struct spdk_bdev_io *>(task->buff),
                            true, task);
#else
    int w_rc = spdk_bdev_write_blocks(
        bdev->spBdevCtx.bdev_desc, bdev->spBdevCtx.io_channel, task->buff,
        task->blockSize * (*task->lba), task->blockSize,
        SpdkBdev::writeComplete, task);
#endif

    if (w_rc) {
        stats.write_err_cnt++;
        /*
         * -ENOMEM condition indicates a shoortage of IO buffers.
         *  Schedule this IO for later execution by providing a callback
         * function, when sufficient IO buffers are available
         */
        if (w_rc == -ENOMEM) {
            w_rc = SpdkBdev::reschedule(task);
            if (w_rc) {
                DAQ_CRITICAL("Spdk queue_io_wait error [" +
                             std::to_string(w_rc) + "] for spdk bdev");
                bdev->deinit();
                spdk_app_stop(-1);
            }
        } else {
            DAQ_CRITICAL("Spdk write error [" + std::to_string(w_rc) +
                         "] for spdk bdev");
            bdev->deinit();
            spdk_app_stop(-1);
        }
    } else
        stats.outstanding_io_cnt++;

    return !w_rc ? true : false;
}

int SpdkBdev::reschedule(DeviceTask *task) {
    SpdkBdev *bdev = reinterpret_cast<SpdkBdev *>(task->bdev);

    task->bdev_io_wait.bdev = spBdevCtx.bdev;
    task->bdev_io_wait.cb_fn = SpdkBdev::writeQueueIoWait;
    task->bdev_io_wait.cb_arg = task;

    return spdk_bdev_queue_io_wait(
        bdev->spBdevCtx.bdev, bdev->spBdevCtx.io_channel, &task->bdev_io_wait);
}

void SpdkBdev::deinit() {
    spdk_put_io_channel(spBdevCtx.io_channel);
    spdk_bdev_close(spBdevCtx.bdev_desc);
}

bool SpdkBdev::init(const SpdkConf &conf) {
    SpdkDevice::init(conf);

    spBdevCtx.bdev_name = conf.bdev_name.c_str();
    spBdevCtx.bdev = 0;
    spBdevCtx.bdev_desc = 0;
    spdk_bdev_opts bdev_opts;

    spBdevCtx.bdev = spdk_bdev_first();
    if (!spBdevCtx.bdev) {
        DAQ_CRITICAL(std::string("No NVMe devices detected for name[") +
                     spBdevCtx.bdev_name + "]");
        spBdevCtx.state = SPDK_BDEV_NOT_FOUND;
        return false;
    }
    DAQ_DEBUG("NVMe devices detected for name[" + spBdevCtx->bdev_name + "]");

    int rc = spdk_bdev_open(spBdevCtx.bdev, true, 0, 0, &spBdevCtx.bdev_desc);
    if (rc) {
        DAQ_CRITICAL("Open BDEV failed with error code[" + std::to_string(rc) + "]");
        spBdevCtx.state = SPDK_BDEV_ERROR;
        return false;
    }

    spdk_bdev_get_opts(&bdev_opts);
    bdev_opts.bdev_io_cache_size = bdev_opts.bdev_io_pool_size >> 1;
    spdk_bdev_set_opts(&bdev_opts);
    spdk_bdev_get_opts(&bdev_opts);
    DAQ_DEBUG("bdev.bdev_io_pool_size[" + bdev_opts.bdev_io_pool_size + "]" +
              " bdev.bdev.io_cache_size[" + bdev_opts.bdev_io_cache_size + "]");

    spBdevCtx.io_pool_size = bdev_opts.bdev_io_pool_size;
    spBdevCtx.io_cache_size = bdev_opts.bdev_io_cache_size;

    spBdevCtx.io_channel = spdk_bdev_get_io_channel(spBdevCtx.bdev_desc);
    if (!spBdevCtx.io_channel) {
        DAQ_CRITICAL(std::string("Get io_channel failed bdev[") +
                     spBdevCtx.bdev_name + "]");
        spBdevCtx.state = SPDK_BDEV_ERROR;
        return false;
    }

    spBdevCtx.blk_size = spdk_bdev_get_block_size(spBdevCtx.bdev);
    DAQ_DEBUG("BDEV block size[" + std::to_string(spBdevCtx.blk_size) + "]");

    spBdevCtx.data_blk_size = spdk_bdev_get_data_block_size(spBdevCtx.bdev);
    DAQ_DEBUG("BDEV data block size[" + spBdevCtx.data_blk_size + "]");

    spBdevCtx.buf_align = spdk_bdev_get_buf_align(spBdevCtx.bdev);
    DAQ_DEBUG("BDEV align[" + std::to_string(spBdevCtx.buf_align) + "]");

    spBdevCtx.blk_num = spdk_bdev_get_num_blocks(spBdevCtx.bdev);
    DAQ_DEBUG("BDEV number of blocks[" + std::to_string(spBdevCtx.blk_num) +
              "]");

    return true;
}

void SpdkBdev::setMaxQueued(uint32_t io_cache_size, uint32_t blk_size) {
    IoBytesMaxQueued = io_cache_size * 128;
}

void SpdkBdev::enableStats(bool en) { stats.enableStats(en); }
} // namespace DaqDB
