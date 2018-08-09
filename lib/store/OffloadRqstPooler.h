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

#include "spdk/bdev.h"
#include "spdk/env.h"
#include "spdk/io_channel.h"
#include "spdk/queue.h"

#include "RTreeEngine.h"
#include <FogKV/KVStoreBase.h>
#include "OffloadFreeList.h"

#define OFFLOAD_DEQUEUE_RING_LIMIT 1024

namespace FogKV {

enum class OffloadRqstOperation : std::int8_t {
    NONE = 0,
    GET = 1,
    UPDATE = 2,
    REMOVE = 3
};

class OffloadRqstMsg {
  public:
    OffloadRqstMsg(const OffloadRqstOperation op, const char *key,
                   const size_t keySize, const char *value, size_t valueSize,
                   KVStoreBase::KVStoreBaseCallback clb);

    const OffloadRqstOperation op = OffloadRqstOperation::NONE;
    const char *key = nullptr;
    size_t keySize = 0;
    const char *value = nullptr;
    size_t valueSize = 0;

    // @TODO jradtke copying std:function in every msg might be not good idea
    KVStoreBase::KVStoreBaseCallback clb;
};

struct BdevContext {
    struct spdk_bdev *bdev;
    struct spdk_bdev_desc *bdev_desc;
    struct spdk_io_channel *bdev_io_channel;
    uint32_t blk_size = 0;
    uint32_t buf_align = 0;
    uint64_t blk_num = 0;
    char *bdev_name;
};

struct IoContext {
    char *buff;
    uint32_t size = 0;
    const char *key = nullptr;
    size_t keySize = 0;

    uint64_t *lba = nullptr;
    bool updatePmemIOV = false;

    std::shared_ptr<FogKV::RTreeEngine> rtree;
    KVStoreBase::KVStoreBaseCallback clb;
};


class OffloadRqstPoolerInterface {
    virtual bool EnqueueMsg(OffloadRqstMsg *Message) = 0;
    virtual void DequeueMsg() = 0;
    virtual void ProcessMsg() = 0;
    virtual bool Read(IoContext *ioCtx) = 0;
    virtual bool Write(IoContext *ioCtx) = 0;
    virtual int64_t GetFreeLba() = 0;
};

class OffloadRqstPooler : public OffloadRqstPoolerInterface {
  public:
    OffloadRqstPooler(std::shared_ptr<FogKV::RTreeEngine> rtree,
                      BdevContext &bdevContext, uint64_t offloadBlockSize);
    virtual ~OffloadRqstPooler();

    bool EnqueueMsg(OffloadRqstMsg *Message);
    void DequeueMsg();
    void ProcessMsg() final;

    bool Read(IoContext *ioCtx);
    bool Write(IoContext *ioCtx);
    int64_t GetFreeLba();

    void InitFreeList();
    void SetBdevContext(BdevContext &_bdevContext);

    struct spdk_ring *rqstRing;

    unsigned short processArrayCount = 0;
    OffloadRqstMsg *processArray[OFFLOAD_DEQUEUE_RING_LIMIT];

    std::shared_ptr<FogKV::RTreeEngine> rtree;

    OffloadFreeList *freeLbaList = nullptr;

  private:
    uint64_t _blkForLba = 0;
    struct BdevContext _bdevContext;
    pool<FogKV::OffloadFreeList> _poolFreeList;
};

} /* namespace FogKV */
