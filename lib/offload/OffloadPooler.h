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

#include <daqdb/KVStoreBase.h>
#include <RTreeEngine.h>
#include <Rqst.h>
#include <Pooler.h>
#include "OffloadFreeList.h"

#define OFFLOAD_DEQUEUE_RING_LIMIT 1024

namespace DaqDB {

struct BdevCtx {
    struct spdk_bdev *bdev;
    struct spdk_bdev_desc *bdev_desc;
    struct spdk_io_channel *io_channel;
    uint32_t blk_size = 0;
    uint32_t buf_align = 0;
    uint64_t blk_num = 0;
    char *bdev_name;
};

struct IoCtx {
    char *buff;
    size_t size = 0;
    uint32_t blockSize = 0;
    const char *key = nullptr;
    size_t keySize = 0;
    uint64_t *lba = nullptr; // pointer used to store pmem allocated memory
    bool updatePmemIOV = false;
    std::shared_ptr<DaqDB::RTreeEngine> rtree;
    KVStoreBase::KVStoreBaseCallback clb;
};

enum class OffloadOperation : std::int8_t { NONE = 0, GET, UPDATE, REMOVE };
using OffloadRqst = Rqst<OffloadOperation>;

class OffloadPooler : public Pooler<OffloadRqst> {
  public:
    OffloadPooler(std::shared_ptr<DaqDB::RTreeEngine> &rtree,
                  BdevCtx &bdevContext, uint64_t offloadBlockSize);
    virtual ~OffloadPooler() {};

    void Process() final;

    virtual bool Read(IoCtx *ioCtx);
    virtual bool Write(IoCtx *ioCtx);
    virtual int64_t GetFreeLba();

    void InitFreeList();
    void SetBdevContext(BdevCtx &_bdevContext);

    std::shared_ptr<DaqDB::RTreeEngine> rtree;

    OffloadFreeList *freeLbaList = nullptr;

  private:
    void _ProcessGet(const OffloadRqst *rqst);
    void _ProcessUpdate(const OffloadRqst *rqst);
    void _ProcessRemove(const OffloadRqst *rqst);

    StatusCode _GetValCtx(const OffloadRqst *rqst, ValCtx &valCtx) const;

    inline void _RqstClb(const OffloadRqst *rqst, StatusCode status) {
        if (rqst->clb)
            rqst->clb(nullptr, status, rqst->key, rqst->keySize, nullptr, 0);
    }

    inline bool _ValOffloaded(ValCtx &valCtx) {
        return valCtx.location == LOCATIONS::DISK;
    }
    inline bool _ValInPmem(ValCtx &valCtx) {
        return valCtx.location == LOCATIONS::PMEM;
    }
    inline size_t _GetAlignedSize(ValCtx &valCtx) {
        return valCtx.size + _bdevCtx.blk_size - 1 & ~(_bdevCtx.blk_size - 1);
    }
    inline size_t _GetAlignedSize(size_t size) {
            return size + _bdevCtx.blk_size - 1 & ~(_bdevCtx.blk_size - 1);
        }
    inline uint32_t _GetSizeInBlk(size_t &size) {
        return size / _bdevCtx.blk_size;
    }

    uint64_t _blkForLba = 0;
    struct BdevCtx _bdevCtx;
    pool<DaqDB::OffloadFreeList> _poolFreeList;
};

} // namespace DaqDB
