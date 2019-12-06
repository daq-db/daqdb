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

SpdkRAID0Bdev::SpdkRAID0Bdev() {}

int SpdkRAID0Bdev::read(DeviceTask *task) { return 0; }

int SpdkRAID0Bdev::write(DeviceTask *task) { return 0; }

int SpdkRAID0Bdev::reschedule(DeviceTask *task) { return 0; }

void SpdkRAID0Bdev::deinit() {}

bool SpdkRAID0Bdev::init(const SpdkConf &conf) { return true; }

void SpdkRAID0Bdev::enableStats(bool en) {}

} // namespace DaqDB
