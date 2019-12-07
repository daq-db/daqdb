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

#include <iostream>
#include <sstream>
#include <string>
#include <atomic>
#include <cstdint>
#include <thread>
#include <mutex>
#include <chrono>
#include <condition_variable>

#include "spdk/bdev.h"
#include "spdk/env.h"
#include "spdk/io_channel.h"
#include "spdk/queue.h"

#include "OffloadFreeList.h"
#include <Poller.h>
#include <RTreeEngine.h>
#include <Rqst.h>
#include <SpdkBdevFactory.h>
#include <SpdkCore.h>
#include <daqdb/KVStoreBase.h>

#define OFFLOAD_DEQUEUE_RING_LIMIT 1024

namespace DaqDB {

class OffloadPoller : public Poller<OffloadRqst> {
  public:
    OffloadPoller(RTreeEngine *rtree, SpdkCore *_spdkCore);
    virtual ~OffloadPoller();

    void process() final;
    virtual int64_t getFreeLba();

    void initFreeList();

    inline bool isValOffloaded(ValCtx &valCtx) {
        return valCtx.location == LOCATIONS::DISK;
    }
    inline bool isValInPmem(ValCtx &valCtx) {
        return valCtx.location == LOCATIONS::PMEM;
    }

    virtual SpdkBdev *getBdev() {
        return dynamic_cast<SpdkBdev *>(spdkCore->spBdev.get());
    }

    virtual SpdkBdevCtx *getBdevCtx() {
        return &dynamic_cast<SpdkBdev *>(spdkCore->spBdev.get())->spBdevCtx;
    }

    inline spdk_bdev_desc *getBdevDesc() {
        return dynamic_cast<SpdkBdev *>(spdkCore->spBdev.get())
            ->spBdevCtx.bdev_desc;
    }

    inline spdk_io_channel *getBdevIoChannel() {
        return dynamic_cast<SpdkBdev *>(spdkCore->spBdev.get())
            ->spBdevCtx.io_channel;
    }

    RTreeEngine *rtree;
    SpdkCore *spdkCore;

    OffloadFreeList *freeLbaList = nullptr;

    std::atomic<int> isRunning;

    void signalReady();
    bool waitReady();

    virtual void setRunning(int rn) { isRunning = rn; }
    virtual bool isOffloadRunning() { return isRunning; }

  private:
    void _spdkThreadMain(void);

    void _processGet(OffloadRqst *rqst);
    void _processUpdate(OffloadRqst *rqst);
    void _processRemove(OffloadRqst *rqst);

    StatusCode _getValCtx(const OffloadRqst *rqst, ValCtx &valCtx) const;

    inline void _rqstClb(const OffloadRqst *rqst, StatusCode status) {
        if (rqst->clb)
            rqst->clb(nullptr, status, rqst->key, rqst->keySize, nullptr, 0);
    }

    pool<DaqDB::OffloadFreeList> _offloadFreeList;

    std::string bdevName;
    const static char *pmemFreeListFilename;
};

} // namespace DaqDB
