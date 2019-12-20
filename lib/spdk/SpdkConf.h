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

#include "RTreeEngine.h"

namespace DaqDB {

typedef OffloadDevType SpdkConfDevType;

struct SpdkBdevConf {
    std::string devName = "";
    std::string nvmeAddr = "";
    struct PciAddr pciAddr;
    std::string nvmeName = "";
};

/*
 * Encapsulates Spdk config
 */
class SpdkConf {
  public:
    SpdkConf(const OffloadOptions &_offloadOptions);
    SpdkConf(SpdkConfDevType devType, std::string name, size_t raid0StripeSize);
    ~SpdkConf() = default;

    SpdkConf &operator=(const SpdkConf &_r);

    struct PciAddr parsePciAddr(const std::string &nvmeAddr);
    void copyDevs(const std::vector<OffloadDevDescriptor> &_offloadDevs);

    const std::string &getBdevNvmeName() const;
    const std::string &getBdevNvmeAddr() const;
    struct PciAddr getBdevSpdkPciAddr();

    SpdkConfDevType getSpdkConfDevType() const { return _devType; }
    void setSpdkConfDevType(SpdkConfDevType devType) { _devType = devType; }
    std::string getName() { return _name; }
    void setName(std::string &name) { _name = name; }
    size_t getRaid0StripeSize() { return _raid0StripeSize; }
    void setRaid0StripeSize(size_t raid0StripeSize) {
        _raid0StripeSize = raid0StripeSize;
    }
    struct spdk_bdev *getBdev() const {
        return _bdev;
    }
    void setBdev(struct spdk_bdev *bdev) { _bdev = bdev; }
    const std::vector<SpdkBdevConf> &getDevs() const { return _devs; }
    void addDev(SpdkBdevConf dev);
    int getBdevNum() const { return _bdevNum; }
    void setBdevNum(int bdevNum) { _bdevNum = bdevNum; }

  private:
    SpdkConfDevType _devType;
    std::string _name;
    size_t _raid0StripeSize;
    std::vector<SpdkBdevConf> _devs;
    struct spdk_bdev *_bdev;
    int _bdevNum;
};

} // namespace DaqDB
