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

#include "spdk/bdev.h"

#include "SpdkJBODBdev.h"
#include <Logger.h>

namespace DaqDB {

SpdkDeviceClass SpdkJBODBdev::bdev_class = SpdkDeviceClass::JBOD;

SpdkJBODBdev::SpdkJBODBdev(bool _statsEnabled)
    : statsEnabled(_statsEnabled), isRunning(0) {}

SpdkJBODBdev::~SpdkJBODBdev() {
    for (; numDevices; numDevices--) {
        devices[numDevices].bdev->setRunning(0);
        delete devices[numDevices].bdev;
    }
}

int SpdkJBODBdev::read(DeviceTask *task) {
    for (uint32_t i = 0; i < numDevices; i++) {
        if (task->bdevAddr->busAddr.spdkPciAddr ==
            devices[i].addr.busAddr.spdkPciAddr)
            return devices[i].bdev->read(task);
    }
    return -1;
}

int SpdkJBODBdev::write(DeviceTask *task) {
    int ret = devices[currDevice].bdev->write(task);
    currDevice++;
    currDevice %= numDevices;
    return ret;
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

    struct spdk_bdev *bdev = spdk_bdev_first_leaf();

    for (auto d : conf.getDevs()) {
        devices[numDevices].addr.lba = -1; // max
        devices[numDevices].addr.busAddr.spdkPciAddr = d.pciAddr;
        devices[numDevices].num = numDevices;
        devices[numDevices].bdev = new SpdkBdev(statsEnabled);

        SpdkConf currConf(SpdkConfDevType::BDEV, d.devName, 0);
        currConf.setBdev(bdev);

        currConf.addDev(d);
        bool ret = devices[numDevices].bdev->init(currConf);
        if (ret == false) {
            return false;
        }
        spBdevCtx = devices[numDevices].bdev->spBdevCtx;
        setBlockNumForLba(devices[numDevices].bdev->_blkNumForLba);
        numDevices++;
        bdev = spdk_bdev_next_leaf(bdev);
    }
    return true;
}

void SpdkJBODBdev::enableStats(bool en) {}

void SpdkJBODBdev::setMaxQueued(uint32_t io_cache_size, uint32_t blk_size) {
    IoBytesMaxQueued = io_cache_size * 128;
}

} // namespace DaqDB
