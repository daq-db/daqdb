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

#include "SpdkRAID0Bdev.h"
#include <Logger.h>

namespace DaqDB {

SpdkDeviceClass SpdkRAID0Bdev::bdev_class = SpdkDeviceClass::RAID0;

SpdkRAID0Bdev::SpdkRAID0Bdev() : isRunning(0) {}

bool SpdkRAID0Bdev::read(DeviceTask *task) { return true; }

bool SpdkRAID0Bdev::write(DeviceTask *task) { return true; }

bool SpdkRAID0Bdev::remove(DeviceTask *task) { return true; }

int SpdkRAID0Bdev::reschedule(DeviceTask *task) { return 0; }

void SpdkRAID0Bdev::deinit() {}

bool SpdkRAID0Bdev::init(const SpdkConf &conf) { return true; }

void SpdkRAID0Bdev::initFreeList() {}

int64_t SpdkRAID0Bdev::getFreeLba(size_t ioSize) { return -1; }

void SpdkRAID0Bdev::putFreeLba(const DeviceAddr *devAddr, size_t ioSize) {}

void SpdkRAID0Bdev::enableStats(bool en) {}

void SpdkRAID0Bdev::setMaxQueued(uint32_t io_cache_size, uint32_t blk_size) {
    IoBytesMaxQueued = io_cache_size * 128;
}

} // namespace DaqDB
