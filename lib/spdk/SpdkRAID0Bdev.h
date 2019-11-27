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

#include "Rqst.h"
#include "SpdkDevice.h"

namespace DaqDB {

class SpdkRAID0Bdev : public SpdkDevice<OffloadRqst> {
  public:
    SpdkRAID0Bdev(void);
    ~SpdkRAID0Bdev() = default;

    /**
     * Initialize RAID0 devices.
     *
     * @return  if this RAID0 devices successfully configured and opened, false
     * otherwise
     */
    bool init();
    void deinit();

    /*
     * SpdkDevice virtual interface
     */
    virtual int read(OffloadRqst *rqst);
    virtual int write(OffloadRqst *rqst);
};

} // namespace DaqDB
