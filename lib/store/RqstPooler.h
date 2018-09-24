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

#include "OffloadPooler.h"
#include "RTreeEngine.h"
#include "spdk/env.h"
#include "spdk/io_channel.h"
#include "spdk/queue.h"

#include <daqdb/KVStoreBase.h>

#define DEQUEUE_RING_LIMIT 1024

namespace DaqDB {

enum class RqstOperation : std::int8_t { NONE = 0, GET = 1, PUT = 2, UPDATE = 3 };

class RqstMsg {
  public:
    RqstMsg(const RqstOperation op, const char *key, const size_t keySize,
            const char *value, size_t valueSize,
            KVStoreBase::KVStoreBaseCallback clb);

    const RqstOperation op = RqstOperation::NONE;
    const char *key = nullptr;
    size_t keySize = 0;
    const char *value = nullptr;
    size_t valueSize = 0;

    // @TODO jradtke copying std:function in every msg might be not good idea
    KVStoreBase::KVStoreBaseCallback clb;
};

class RqstPoolerInterface {
    virtual void StartThread() = 0;

    virtual bool EnqueueMsg(RqstMsg *Message) = 0;
    virtual void DequeueMsg() = 0;
    virtual void ProcessMsg() = 0;
};

class RqstPooler : public RqstPoolerInterface {
  public:
    RqstPooler(std::shared_ptr<DaqDB::RTreeEngine> &rtree,
               const size_t cpuCore = 0);
    virtual ~RqstPooler();

    bool EnqueueMsg(RqstMsg *Message);
    void DequeueMsg();
    void ProcessMsg() final;
    void StartThread();

    OffloadPooler *offloadPooler = nullptr;

    std::atomic<int> isRunning;
    std::shared_ptr<DaqDB::RTreeEngine> rtree;
    struct spdk_ring *rqstRing;

    unsigned short processArrayCount = 0;
    RqstMsg *processArray[DEQUEUE_RING_LIMIT];

  private:
    void _ThreadMain(void);

    std::thread *_thread;
    size_t _cpuCore = 0;
};

} /* namespace FogKV */
