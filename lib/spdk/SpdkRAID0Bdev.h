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

#include <atomic>
#include <cstdint>
#include <memory>

#include "spdk/bdev.h"

#include "Rqst.h"
#include "SpdkConf.h"
#include "SpdkDevice.h"
#include <Logger.h>
#include <RTree.h>

namespace DaqDB {

class SpdkRAID0Bdev : public SpdkDevice<DeviceTask<SpdkRAID0Bdev>> {
  public:
    SpdkRAID0Bdev(void);
    ~SpdkRAID0Bdev() = default;

    /**
     * Initialize RAID0 devices.
     *
     * @return  if this RAID0 devices successfully configured and opened, false
     * otherwise
     */
    virtual bool init(const SpdkConf &conf);
    virtual void deinit();

    /*
     * SpdkDevice virtual interface
     */
    virtual int read(DeviceTask<SpdkRAID0Bdev> *task);
    virtual int write(DeviceTask<SpdkRAID0Bdev> *task);
    virtual int reschedule(DeviceTask<SpdkRAID0Bdev> *task);
};

} // namespace DaqDB
