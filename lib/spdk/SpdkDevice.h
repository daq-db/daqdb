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

/*
 * Pure abstract class to define read/write IO interface
 */
class SpdkDevice {
  public:
    SpdkDevice() = default;
    virtual ~SpdkDevice() = default;

    virtual int write(DeviceTask *task) = 0;
    virtual int read(DeviceTask *task) = 0;
    virtual int reschedule(DeviceTask *task) = 0;

    virtual void enableStats(bool en) = 0;

    virtual bool init(const SpdkConf &_conf) = 0;
    virtual void deinit() = 0;
    inline virtual size_t getOptimalSize(size_t size) = 0;
    inline virtual size_t getAlignedSize(size_t size) = 0;
    inline virtual uint32_t getSizeInBlk(size_t &size) = 0;
    void virtual setReady() = 0;
    virtual bool isBdevFound() = 0;
    virtual bool isOffloadEnabled() = 0;
    virtual void IOQuiesce() = 0;
    virtual bool isIOQuiescent() = 0;
    virtual void IOAbort() = 0;
};

} // namespace DaqDB
