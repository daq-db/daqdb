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
    PmemPoller(std::shared_ptr<DaqDB::RTreeEngine> &rtree,
               const size_t cpuCore = 0);
    virtual ~PmemPoller();

    void process() final;
    void startThread();

    OffloadPoller *offloadPoller = nullptr;

    std::atomic<int> isRunning;
    std::shared_ptr<DaqDB::RTreeEngine> rtree;

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
