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
#include <pthread.h>
#include <string>
#include <thread>

#include <boost/asio.hpp>
#include <boost/filesystem.hpp>

#include "spdk/stdinc.h"
#include "spdk/cpuset.h"
#include "spdk/queue.h"
#include "spdk/log.h"
#include "spdk/thread.h"
#include "spdk/event.h"
#include "spdk/ftl.h"
#include "spdk/conf.h"
#include "spdk/env.h"

#include "OffloadPoller.h"
#include <Logger.h>
#include <RTree.h>
#include <daqdb/Status.h>

#define POOL_FREELIST_SIZE 1ULL * 1024 * 1024 * 1024
#define LAYOUT "queue"
#define CREATE_MODE_RW (S_IWUSR | S_IRUSR)

namespace DaqDB {

const char *OffloadPoller::pmemFreeListFilename = "/mnt/pmem/offload_free.pm";

/*
 * Callback function for write io completion.
 */
void OffloadPoller::writeComplete(struct spdk_bdev_io *bdev_io, bool success,
                           void *cb_arg) {
    spdk_bdev_free_io(bdev_io);
    OffloadIoCtx *ioCtx = reinterpret_cast<OffloadIoCtx *>(cb_arg);

    if (success) {
        if (ioCtx->updatePmemIOV)
            ioCtx->rtree->UpdateValueWrapper(ioCtx->key, ioCtx->lba,
                                             sizeof(uint64_t));
        if (ioCtx->clb)
            ioCtx->clb(nullptr, StatusCode::OK, ioCtx->key, ioCtx->keySize,
                       ioCtx->buff, ioCtx->size);
    } else {
        if (ioCtx->clb)
            ioCtx->clb(nullptr, StatusCode::UNKNOWN_ERROR, ioCtx->key,
                       ioCtx->keySize, nullptr, 0);
    }

    delete ioCtx;
}

/*
 * Callback function for read io completion.
 */
void OffloadPoller::readComplete(struct spdk_bdev_io *bdev_io, bool success,
                          void *cb_arg) {
    spdk_bdev_free_io(bdev_io);
    OffloadIoCtx *ioCtx = reinterpret_cast<OffloadIoCtx *>(cb_arg);

    if (success) {
        if (ioCtx->clb)
            ioCtx->clb(nullptr, StatusCode::OK, ioCtx->key, ioCtx->keySize,
                       ioCtx->buff, ioCtx->size);
    } else {
        if (ioCtx->clb)
            ioCtx->clb(nullptr, StatusCode::UNKNOWN_ERROR, ioCtx->key,
                       ioCtx->keySize, nullptr, 0);
    }

    delete ioCtx;
}

OffloadPoller::OffloadPoller(RTreeEngine *rtree, SpdkCore *spdkCore,
                             const size_t cpuCore):
    Poller<OffloadRqst>(false),
    rtree(rtree), spdkCore(spdkCore), _spdkThread(0), _loopThread(0), _cpuCore(cpuCore) {
    _syncLock = new std::unique_lock<std::mutex>(_syncMutex);

    if (spdkCore->isSpdkReady() == true ) {
        startSpdkThread();
    }
}

OffloadPoller::~OffloadPoller() {
    isRunning = 0;
    if (_spdkThread != nullptr)
        _spdkThread->join();
    if ( _loopThread )
        _loopThread->join();
}

void OffloadPoller::startSpdkThread() {
    _spdkThread = new std::thread(&OffloadPoller::_spdkThreadMain, this);
    DAQ_DEBUG("OffloadPoller thread started");
    if (_cpuCore) {
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(_cpuCore, &cpuset);

        const int set_result = pthread_setaffinity_np(
            _spdkThread->native_handle(), sizeof(cpu_set_t), &cpuset);
        if (set_result == 0) {
            DAQ_DEBUG("OffloadPoller thread affinity set on CPU core [" +
                      std::to_string(_cpuCore) + "]");
        } else {
            DAQ_DEBUG("Cannot set affinity on CPU core [" +
                      std::to_string(_cpuCore) + "] for OffloadReactor");
        }
    }
}

bool OffloadPoller::read(OffloadIoCtx *ioCtx) {
    int r_rc = spdk_bdev_read_blocks(getBdevDesc(), getBdevIoChannel(), ioCtx->buff,
                                 getBlockOffsetForLba(*ioCtx->lba),
                                 ioCtx->blockSize, OffloadPoller::readComplete, ioCtx);
    if ( r_rc ) {
        DAQ_CRITICAL("Spdk read error [" + std::to_string(r_rc) + "] for offload bdev");
    }
    return r_rc;
}

bool OffloadPoller::write(OffloadIoCtx *ioCtx) {
    int w_rc = spdk_bdev_write_blocks(getBdevDesc(), getBdevIoChannel(),
                                  ioCtx->buff,
                                  getBlockOffsetForLba(*ioCtx->lba),
                                  ioCtx->blockSize, OffloadPoller::writeComplete, ioCtx);
    if ( w_rc ) {
        DAQ_CRITICAL("Spdk write error [" + std::to_string(w_rc) + "] for offload bdev");
    }
    return w_rc;
}

void OffloadPoller::initFreeList() {
    auto initNeeded = false;
    if (getBdevCtx()) {
        if (boost::filesystem::exists(OffloadPoller::pmemFreeListFilename)) {
            _offloadFreeList =
                pool<OffloadFreeList>::open(OffloadPoller::pmemFreeListFilename, LAYOUT);
        } else {
            _offloadFreeList = pool<OffloadFreeList>::create(
                    OffloadPoller::pmemFreeListFilename, LAYOUT, POOL_FREELIST_SIZE,
                CREATE_MODE_RW);
            initNeeded = true;
        }
        freeLbaList = _offloadFreeList.get_root().get();
        freeLbaList->maxLba = getBdevCtx()->blk_num / _blkNumForLba;
        if (initNeeded) {
            freeLbaList->push(_offloadFreeList, -1);
        }
    }
}

StatusCode OffloadPoller::_getValCtx(const OffloadRqst *rqst,
                                     ValCtx &valCtx) const {
    try {
        rtree->Get(rqst->key, rqst->keySize, &valCtx.val, &valCtx.size,
                   &valCtx.location);
    } catch (...) {
        /** @todo fix exception handling */
        return StatusCode::UNKNOWN_ERROR;
    }
    return StatusCode::OK;
}

void OffloadPoller::_processGet(const OffloadRqst *rqst) {
    ValCtx valCtx;

    auto rc = _getValCtx(rqst, valCtx);
    if (rc != StatusCode::OK) {
        _rqstClb(rqst, rc);
        return;
    }

    if (isValInPmem(valCtx)) {
        _rqstClb(rqst, StatusCode::KEY_NOT_FOUND);
        return;
    }

    auto size = getBdev()->getAlignedSize(valCtx.size);

    char *buff = reinterpret_cast<char *>(spdk_dma_zmalloc(size, getBdevCtx()->buf_align, NULL));

    auto blkSize = getBdev()->getSizeInBlk(size);
    OffloadIoCtx *ioCtx = new OffloadIoCtx{
        buff,      valCtx.size,   blkSize,
        rqst->key, rqst->keySize, static_cast<uint64_t *>(valCtx.val),
        false,     rtree,         rqst->clb};

    if (read(ioCtx) != 0)
        _rqstClb(rqst, StatusCode::UNKNOWN_ERROR);
}

void OffloadPoller::_processUpdate(const OffloadRqst *rqst) {
    OffloadIoCtx *ioCtx = nullptr;
    ValCtx valCtx;

    if (rqst == nullptr) {
        _rqstClb(rqst, StatusCode::UNKNOWN_ERROR);
        return;
    }

    auto rc = _getValCtx(rqst, valCtx);
    if (rc != StatusCode::OK) {
        _rqstClb(rqst, rc);
        return;
    }

    if (isValInPmem(valCtx)) {

        const char *val;
        size_t valSize = 0;
        if (rqst->valueSize > 0) {
            val = rqst->value;
            valSize = rqst->valueSize;
        } else {
            val = static_cast<char *>(valCtx.val);
            valSize = valCtx.size;
        }

        auto valSizeAlign = getBdev()->getAlignedSize(valSize);
        auto buff = reinterpret_cast<char *>(
            spdk_dma_zmalloc(valSizeAlign, getBdevCtx()->buf_align, NULL));

        memcpy(buff, val, valSize);
        ioCtx = new OffloadIoCtx{
            buff,      valSize,       getBdev()->getSizeInBlk(valSizeAlign),
            rqst->key, rqst->keySize, nullptr,
            true,      rtree,         rqst->clb};
        try {
            rtree->AllocateIOVForKey(rqst->key, &ioCtx->lba, sizeof(uint64_t));
        } catch (...) {
            /** @todo fix exception handling */
            delete ioCtx;
            _rqstClb(rqst, StatusCode::UNKNOWN_ERROR);
            return;
        }
        *ioCtx->lba = getFreeLba();

    } else if (isValOffloaded(valCtx)) {
        if (valCtx.size == 0) {
            _rqstClb(rqst, StatusCode::OK);
            return;
        }
        auto valSizeAlign = getBdev()->getAlignedSize(rqst->valueSize);
        auto buff = reinterpret_cast<char *>(
            spdk_dma_zmalloc(valSizeAlign, getBdevCtx()->buf_align, NULL));
        memcpy(buff, rqst->value, rqst->valueSize);

        ioCtx = new OffloadIoCtx{
            buff,      rqst->valueSize, getBdev()->getSizeInBlk(valSizeAlign),
            rqst->key, rqst->keySize,   new uint64_t,
            false,     rtree,           rqst->clb};
        *ioCtx->lba = *(static_cast<uint64_t *>(valCtx.val));

    } else {
        _rqstClb(rqst, StatusCode::KEY_NOT_FOUND);
        return;
    }

    if (write(ioCtx) != 0)
        _rqstClb(rqst, StatusCode::UNKNOWN_ERROR);
}

void OffloadPoller::_processRemove(const OffloadRqst *rqst) {

    ValCtx valCtx;

    auto rc = _getValCtx(rqst, valCtx);
    if (rc != StatusCode::OK) {
        _rqstClb(rqst, rc);
        return;
    }

    if (isValInPmem(valCtx)) {
        _rqstClb(rqst, StatusCode::KEY_NOT_FOUND);
        return;
    }

    uint64_t lba = *(static_cast<uint64_t *>(valCtx.val));

    freeLbaList->push(_offloadFreeList, lba);
    rtree->Remove(rqst->key);
    _rqstClb(rqst, StatusCode::OK);
}

void OffloadPoller::process() {
    if (requestCount > 0) {
        for (unsigned short RqstIdx = 0; RqstIdx < requestCount; RqstIdx++) {
            OffloadRqst *rqst = requests[RqstIdx];
            switch (rqst->op) {
            case OffloadOperation::GET:
                _processGet(const_cast<const OffloadRqst *>(rqst));
                break;
            case OffloadOperation::UPDATE: {
                _processUpdate(const_cast<const OffloadRqst *>(rqst));
                break;
            }
            case OffloadOperation::REMOVE: {
                _processRemove(const_cast<const OffloadRqst *>(rqst));
                break;
            }
            default:
                break;
            }
            delete requests[RqstIdx];
        }
        requestCount = 0;
    }
}

void OffloadPoller::spdkStart(void *arg) {
    OffloadPoller *poller = reinterpret_cast<OffloadPoller *>(arg);
    SpdkBdevCtx *bdev_c = poller->getBdev()->spBdevCtx.get();

    bool rc = poller->getBdev()->init(poller->bdevName.c_str());
    if ( rc == false ) {
        DAQ_CRITICAL("Bdev init failed");
        poller->signalReady();
        spdk_app_stop(-1);
        return;
    }

    auto aligned = poller->getBdev()->getAlignedSize(poller->spdkCore->offloadOptions.allocUnitSize);
    poller->setBlockNumForLba(aligned / bdev_c->blk_size);

    poller->initFreeList();
    bool i_rc = poller->init();
    if ( i_rc == false ) {
        DAQ_CRITICAL("Poller init failed");
        poller->signalReady();
        spdk_app_stop(-1);
        return;
    }

    poller->getBdev()->setReady();
    poller->signalReady();

    poller->_loopThread = new std::thread(&OffloadPoller::_loopThreadMain, poller);
}

void OffloadPoller::_spdkThreadMain(void) {
    struct spdk_app_opts daqdb_opts = {};
    spdk_app_opts_init(&daqdb_opts);
    daqdb_opts.config_file = DEFAULT_SPDK_CONF_FILE.c_str();
    daqdb_opts.name = "daqdb_nvme";

    daqdb_opts.mem_size = 1024;
    daqdb_opts.shm_id = 0;
    daqdb_opts.hugepage_single_segments = 1;
    daqdb_opts.hugedir = SpdkCore::spdkHugepageDirname;

    int rc = spdk_app_start(&daqdb_opts, OffloadPoller::spdkStart, this);
    if ( rc ) {
        DAQ_DEBUG("Error spdk_app_start[" + std::to_string(rc) + "]");
        return;
    }
    DAQ_DEBUG("spdk_app_start[" << rc << "]");
}

void OffloadPoller::signalReady() {
    delete _syncLock;
}

void OffloadPoller::waitReady() {
    std::unique_lock<std::mutex> lk(_syncMutex);
}

void OffloadPoller::_loopThreadMain(void) {
    isRunning = 1;
    while (isRunning) {
        dequeue();
        process();
    }
}

int64_t OffloadPoller::getFreeLba() {
    return freeLbaList->get(_offloadFreeList);
}

} // namespace DaqDB
