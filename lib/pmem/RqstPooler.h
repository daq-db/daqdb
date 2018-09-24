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

#include "OffloadPooler.h"
#include "RTreeEngine.h"
#include "Rqst.h"

#define DEQUEUE_RING_LIMIT 1024

namespace DaqDB {

class RqstPoolerMockInterface { // @suppress("Class has a virtual method and non-virtual destructor")
    virtual void StartThread() = 0;

    virtual bool Enqueue(Rqst *rqst) = 0;
    virtual void Dequeue() = 0;
    virtual void Process() = 0;
};

class RqstPooler : public RqstPoolerMockInterface {
  public:
    RqstPooler(std::shared_ptr<DaqDB::RTreeEngine> &rtree,
               const size_t cpuCore = 0);
    virtual ~RqstPooler();

    bool Enqueue(Rqst *rqst);
    void Dequeue();
    void Process() final;
    void StartThread();

    OffloadPooler *offloadPooler = nullptr;

    std::atomic<int> isRunning;
    std::shared_ptr<DaqDB::RTreeEngine> rtree;

    struct spdk_ring *rqstRing;

    unsigned short requestCount = 0;
    std::vector<Rqst *> requests;

  private:
    void _ThreadMain(void);

    void _ProcessGet(const Rqst *rqst);
    void _ProcessPut(const Rqst *rqst);
    void _ProcessTransfer(const Rqst *rqst);

    inline void _RqstClb(const Rqst *rqst, StatusCode status) {
        if (rqst->clb)
            rqst->clb(nullptr, status, rqst->key, rqst->keySize, nullptr, 0);
    }
    inline void _RqstClb(const Rqst *rqst, StatusCode status, Value &val) {
        if (rqst->clb)
            rqst->clb(nullptr, status, rqst->key, rqst->keySize, val.data(),
                      val.size());
    }

    inline bool _ValOffloaded(ValCtx &valCtx) {
        return valCtx.location == LOCATIONS::DISK;
    }
    inline bool _ValInPmem(ValCtx &valCtx) {
        return valCtx.location == LOCATIONS::PMEM;
    }

    std::thread *_thread;
    size_t _cpuCore = 0;
};

}
