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

#include "spdk/env.h"
#include "spdk/io_channel.h"
#include "spdk/queue.h"

#include "../offload/OffloadPoller.h"
#include "RTreeEngine.h"
#include <Poller.h>
#include <Rqst.h>

#define DEQUEUE_RING_LIMIT 1024

namespace DaqDB {

enum class RqstOperation : std::int8_t { NONE = 0, GET, PUT, UPDATE };
using PmemRqst = Rqst<RqstOperation>;

class PmemPoller : public Poller<PmemRqst> {
  public:
    PmemPoller(RTreeEngine *rtree, const size_t cpuCore = 0);
    virtual ~PmemPoller();

    void process() final;
    void startThread();

    OffloadPoller *offloadPoller = nullptr;

    std::atomic<int> isRunning;
    RTreeEngine *rtree;

  private:
    void _threadMain(void);

    void _processGet(const PmemRqst *rqst);
    void _processPut(const PmemRqst *rqst);
    void _processTransfer(const PmemRqst *rqst);

    inline void _rqstClb(const PmemRqst *rqst, StatusCode status) {
        if (rqst->clb)
            rqst->clb(nullptr, status, rqst->key, rqst->keySize, nullptr, 0);
    }
    inline void _rqstClb(const PmemRqst *rqst, StatusCode status, Value &val) {
        if (rqst->clb)
            rqst->clb(nullptr, status, rqst->key, rqst->keySize, val.data(),
                      val.size());
    }

    inline bool _valOffloaded(ValCtx &valCtx) {
        return valCtx.location == LOCATIONS::DISK;
    }
    inline bool _valInPmem(ValCtx &valCtx) {
        return valCtx.location == LOCATIONS::PMEM;
    }

    std::thread *_thread;
    size_t _cpuCore = 0;
};

} // namespace DaqDB
