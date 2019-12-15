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

#include "spdk/bdev.h"
#include "spdk/env.h"

#include <Logger.h>
#include <Options.h>
#include <RTree.h>

namespace DaqDB {

enum class OffloadOperation : std::int8_t { NONE = 0, GET, UPDATE, REMOVE };
using OffloadRqst = Rqst<OffloadOperation>;

typedef OffloadDevType SpdkDeviceClass;

class SpdkDevice;

struct DeviceAddr {
    uint64_t lba;
    union BusAddr {
        uint64_t busAddr;
        struct spdk_pci_addr spdkPciAddr;
    } busAddr __attribute__((packed));
};

inline bool operator==(const struct spdk_pci_addr &l,
                       const struct spdk_pci_addr &r) {
    return l.domain == r.domain && l.bus == r.bus && l.dev == r.dev &&
           l.func == r.func;
}

struct DeviceTask {
  public:
    char *buff;
    size_t size = 0;
    uint32_t blockSize = 0;
    size_t keySize = 0;
    DeviceAddr *bdevAddr =
        nullptr; // pointer used to store pmem allocated memory
    bool updatePmemIOV = false;

    RTreeEngine *rtree;
    KVStoreBase::KVStoreBaseCallback clb;

    SpdkDevice *bdev = nullptr;
    OffloadRqst *rqst = nullptr;
    OffloadOperation op;
    struct spdk_bdev_io_wait_entry bdev_io_wait;
    char key[64];
    bool result;
};

extern "C" enum CSpdkBdevState {
    SPDK_BDEV_INIT = 0,
    SPDK_BDEV_NOT_FOUND,
    SPDK_BDEV_READY,
    SPDK_BDEV_ERROR
};

#define MAX_BDEV_NAME 32
#define BDEV_PCI_ADDR 32
extern "C" struct SpdkBdevCtx {
    spdk_bdev *bdev;
    spdk_bdev_desc *bdev_desc;
    spdk_io_channel *io_channel;
    char *buff;
    char bdev_name[MAX_BDEV_NAME];
    char bdev_addr[BDEV_PCI_ADDR];
    struct spdk_bdev_io_wait_entry bdev_io_wait;
    uint32_t blk_size = 0;
    uint32_t data_blk_size = 0;
    uint32_t buf_align = 0;
    uint64_t blk_num = 0;
    uint32_t io_pool_size = 0;
    uint32_t io_cache_size = 0;
    uint32_t io_min_size = 4096;
    CSpdkBdevState state;
    struct spdk_pci_addr pci_addr;
};

/*
 * Pure abstract class to define read/write IO interface
 */
class SpdkDevice {
  public:
    SpdkDevice() : IoBytesQueued(0), IoBytesMaxQueued(0) {}
    virtual ~SpdkDevice() = default;

    virtual int write(DeviceTask *task) = 0;
    virtual int read(DeviceTask *task) = 0;
    virtual int reschedule(DeviceTask *task) = 0;

    virtual void enableStats(bool en) = 0;

    virtual bool init(const SpdkConf &_conf) = 0;
    virtual void deinit() = 0;
    virtual size_t getOptimalSize(size_t size) = 0;
    virtual size_t getAlignedSize(size_t size) = 0;
    virtual uint32_t getSizeInBlk(size_t &size) = 0;
    virtual void setReady() = 0;
    virtual bool isBdevFound() = 0;
    virtual bool isOffloadEnabled() = 0;
    virtual void IOQuiesce() = 0;
    virtual bool isIOQuiescent() = 0;
    virtual void IOAbort() = 0;
    virtual uint32_t canQueue() = 0;
    virtual SpdkBdevCtx *getBdevCtx() = 0;
    virtual uint64_t getBlockOffsetForLba(uint64_t lba) = 0;
    virtual void setBlockNumForLba(uint64_t blk_num_flba) {
        _blkNumForLba = blk_num_flba;
    }
    virtual void setMaxQueued(uint32_t io_cache_size, uint32_t blk_size) = 0;
    virtual uint32_t getBlockSize() = 0;
    virtual uint32_t getIoPoolSize() = 0;
    virtual uint32_t getIoCacheSize() = 0;
    virtual void setRunning(int running) = 0;
    virtual bool IsRunning(int running) = 0;

    uint64_t _blkNumForLba = 0;
    SpdkBdevCtx spBdevCtx;
    uint64_t IoBytesQueued;
    uint64_t IoBytesMaxQueued;
};

} // namespace DaqDB
