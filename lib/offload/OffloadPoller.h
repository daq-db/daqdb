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
#include <thread>

#include "spdk/bdev.h"
#include "spdk/env.h"
#include "spdk/io_channel.h"
#include "spdk/queue.h"

#include "OffloadFreeList.h"
#include <Poller.h>
#include <RTreeEngine.h>
#include <Rqst.h>
#include <SpdkCore.h>
#include <daqdb/KVStoreBase.h>

#define OFFLOAD_DEQUEUE_RING_LIMIT 1024

namespace DaqDB {

struct OffloadIoCtx {
    char *buff;
    size_t size = 0;
    uint32_t blockSize = 0;
    const char *key = nullptr;
    size_t keySize = 0;
    uint64_t *lba = nullptr; // pointer used to store pmem allocated memory
    bool updatePmemIOV = false;
    void *arg;

    RTreeEngine *rtree;
    KVStoreBase::KVStoreBaseCallback clb;
};

enum class OffloadOperation : std::int8_t { NONE = 0, GET, UPDATE, REMOVE };
using OffloadRqst = Rqst<OffloadOperation>;

class OffloadPoller : public Poller<OffloadRqst> {
  public:
    OffloadPoller(RTreeEngine *rtree, SpdkCore *spdkCore,
                  const size_t cpuCore = 0);
    virtual ~OffloadPoller();

    void process() final;

    virtual bool read(OffloadIoCtx *ioCtx);
    virtual bool write(OffloadIoCtx *ioCtx);
    virtual int64_t getFreeLba();

    void startThread();
    void initFreeList();

    inline bool isValOffloaded(ValCtx &valCtx) {
        return valCtx.location == LOCATIONS::DISK;
    }
    inline bool isValInPmem(ValCtx &valCtx) {
        return valCtx.location == LOCATIONS::PMEM;
    }

    inline uint64_t getBlockOffsetForLba(uint64_t lba) {
        return lba * _blkNumForLba;
    }

    virtual SpdkBdev *getBdev() { return spdkCore->spBdev.get(); }

    virtual SpdkBdevCtx *getBdevCtx() {
        return spdkCore->spBdev->spBdevCtx.get();
    }

    inline spdk_bdev_desc *getBdevDesc() {
        return spdkCore->spBdev->spBdevCtx->bdev_desc;
    }

    inline spdk_io_channel *getBdevIoChannel() {
        return spdkCore->spBdev->spBdevCtx->io_channel;
    }

    void loop(void);

    static void spdkStart(void *arg);
    static void readComplete(struct spdk_bdev_io *bdev_io, bool success,
            void *cb_arg);
    static void writeComplete(struct spdk_bdev_io *bdev_io, bool success,
            void *cb_arg);

    RTreeEngine *rtree;
    SpdkCore *spdkCore;

    OffloadFreeList *freeLbaList = nullptr;

    std::atomic<int> isRunning;

    void setBlockNumForLba(uint64_t blk_num_flba) {
        _blkNumForLba = blk_num_flba;
    }

  private:
    void _threadMain(void);

    void _processGet(const OffloadRqst *rqst);
    void _processUpdate(const OffloadRqst *rqst);
    void _processRemove(const OffloadRqst *rqst);

    StatusCode _getValCtx(const OffloadRqst *rqst, ValCtx &valCtx) const;

    inline void _rqstClb(const OffloadRqst *rqst, StatusCode status) {
        if (rqst->clb)
            rqst->clb(nullptr, status, rqst->key, rqst->keySize, nullptr, 0);
    }

    uint64_t _blkNumForLba = 0;
    pool<DaqDB::OffloadFreeList> _offloadFreeList;

    std::thread *_thread;
    std::thread *_loopThread;
    size_t _cpuCore;
    std::string bdevName;
};

} // namespace DaqDB
