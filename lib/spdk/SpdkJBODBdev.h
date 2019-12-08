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
#include "SpdkBdev.h"
#include "SpdkConf.h"
#include "SpdkDevice.h"
#include <Logger.h>
#include <RTree.h>

namespace DaqDB {

struct JBODDevice {
    DeviceAddr addr;
    SpdkBdev *bdev;
    uint32_t num;
};

class SpdkJBODBdev : public SpdkDevice {
  public:
    SpdkJBODBdev(bool _statsEnabled = false);
    virtual ~SpdkJBODBdev();

    /**
     * Initialize JBOD devices.
     *
     * @return  if this JBOD devices successfully configured and opened, false
     * otherwise
     */
    virtual bool init(const SpdkConf &conf);
    virtual void deinit();

    /*
     * SpdkDevice virtual interface
     */
    virtual int read(DeviceTask *task);
    virtual int write(DeviceTask *task);
    virtual int reschedule(DeviceTask *task);

    virtual void enableStats(bool en);
    inline virtual size_t getOptimalSize(size_t size) { return 0; }
    inline virtual size_t getAlignedSize(size_t size) { return 0; }
    inline virtual uint32_t getSizeInBlk(size_t &size) { return 0; }
    void virtual setReady() {}

    static SpdkDeviceClass bdev_class;

    bool statsEnabled;

  private:
    const static uint32_t maxDevices = 64;
    JBODDevice devices[maxDevices];
    uint32_t numDevices = 0;
    uint32_t currDevice = 0;
};

} // namespace DaqDB
