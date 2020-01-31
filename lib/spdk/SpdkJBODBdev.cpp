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

#include <unistd.h>

#include "spdk/bdev.h"

#include "SpdkJBODBdev.h"
#include <Logger.h>

namespace DaqDB {

SpdkDeviceClass SpdkJBODBdev::bdev_class = SpdkDeviceClass::JBOD;

SpdkJBODBdev::SpdkJBODBdev(bool _statsEnabled)
    : statsEnabled(_statsEnabled), isRunning(0) {
    for (int i = 0; i < maxHash; i++)
        deviceHash[i] = -1;
}

SpdkJBODBdev::~SpdkJBODBdev() {
    IOQuiesce();
    bool isQuiescent = false;
    for (int i = 0; i < 100; i++) {
        isQuiescent = isIOQuiescent();
        if (isQuiescent == true)
            break;
        usleep(1000);
    }
    for (; numDevices; numDevices--) {
        devices[numDevices - 1].bdev->setRunning(0);
    }
    if (isQuiescent == false)
        IOAbort();

    for (; numDevices; numDevices--) {
        delete devices[numDevices - 1].bdev;
    }
}

bool SpdkJBODBdev::read(DeviceTask *task) {
    if (!isRunning)
        return false;

    int32_t idx = deviceHash[hashAddr(task->bdevAddr)];
    if (idx < 0)
        return false;
    task->bdev = devices[static_cast<uint32_t>(idx)].bdev;
    return devices[static_cast<uint32_t>(idx)].bdev->read(task);
}

bool SpdkJBODBdev::write(DeviceTask *task) {
    if (!isRunning)
        return false;

    task->bdev = devices[currDevice].bdev;
    bool ret = devices[currDevice].bdev->write(task);
    currDevice++;
    currDevice %= numDevices;
    return ret;
}

bool SpdkJBODBdev::remove(DeviceTask *task) {
    if (!isRunning)
        return false;

    for (uint32_t i = 0; i < numDevices; i++) {
        if (task->bdevAddr->busAddr.pciAddr ==
            devices[i].addr.busAddr.pciAddr) {
            task->bdev = devices[i].bdev;
            return devices[i].bdev->remove(task);
        }
    }
    return false;
}

int SpdkJBODBdev::reschedule(DeviceTask *task) { return 0; }

void SpdkJBODBdev::deinit() {
    for (uint32_t i = 0; i < numDevices; i++) {
        devices[i].bdev->deinit();
    }
}

bool SpdkJBODBdev::init(const SpdkConf &conf) {
    if (conf.getSpdkConfDevType() != SpdkConfDevType::JBOD) {
        return false;
    }

    int bdevNum = 0;
    for (auto d : conf.getDevs()) {
        devices[numDevices].addr.lba = -1; // max
        devices[numDevices].addr.busAddr.pciAddr = d.pciAddr;
        devices[numDevices].num = numDevices;
        devices[numDevices].bdev = new SpdkBdev(statsEnabled);

        SpdkConf currConf(SpdkConfDevType::BDEV, d.devName, 0);
        currConf.setBdevNum(bdevNum++);
        currConf.addDev(d);
        bool ret = devices[numDevices].bdev->init(currConf);
        if (ret == false) {
            return false;
        }
        spBdevCtx = devices[numDevices].bdev->spBdevCtx;

        deviceHash[hashAddr(&devices[numDevices].addr)] = numDevices;
        numDevices++;
    }

    for (uint32_t i = 0; i < numDevices; i++) {
        devices[i].bdev->maxCacheIoBufs = spBdevCtx.io_cache_size / numDevices;
        devices[i].bdev->maxIoBufs = spBdevCtx.io_pool_size / numDevices;
    }

    return true;
}

void SpdkJBODBdev::initFreeList() {
    for (uint32_t i = 0; i < numDevices; i++) {
        devices[i].bdev->initFreeList();
    }
}

void SpdkJBODBdev::enableStats(bool en) { statsEnabled = en; }

void SpdkJBODBdev::setMaxQueued(uint32_t io_cache_size, uint32_t blk_size) {}

void SpdkJBODBdev::setBlockNumForLba(uint64_t blk_num_flba) {
    blkNumForLba = blk_num_flba;

    for (uint32_t i = 0; i < numDevices; i++) {
        devices[i].bdev->setBlockNumForLba(blk_num_flba);
    }
}

int64_t SpdkJBODBdev::getFreeLba(size_t ioSize) { return -1; }

void SpdkJBODBdev::putFreeLba(const DeviceAddr *devAddr, size_t ioSize) {
    if (!isRunning)
        return;

    for (uint32_t i = 0; i < numDevices; i++) {
        if (devAddr->busAddr.pciAddr == devices[i].addr.busAddr.pciAddr) {
            devices[i].bdev->putFreeLba(devAddr, ioSize);
            return;
        }
    }
}

uint32_t SpdkJBODBdev::canQueue() { return -1; }

void SpdkJBODBdev::IOQuiesce() {
    for (uint32_t i = 0; i < numDevices; i++) {
        devices[i].bdev->IOQuiesce();
    }
}

bool SpdkJBODBdev::isIOQuiescent() {
    for (uint32_t i = 0; i < numDevices; i++) {
        if (devices[i].bdev->isIOQuiescent() == false)
            return false;
    }
    return true;
}

void SpdkJBODBdev::IOAbort() {
    for (uint32_t i = 0; i < numDevices; i++) {
        devices[i].bdev->IOAbort();
    }
}

} // namespace DaqDB
