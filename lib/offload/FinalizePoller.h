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
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>

#include "spdk/bdev.h"
#include "spdk/env.h"
#include "spdk/io_channel.h"
#include "spdk/queue.h"

#include <BlockingPoller.h>
#include <Poller.h>
#include <RTree.h>
#include <Rqst.h>
#include <SpdkBdev.h>
#include <SpdkConf.h>
#include <SpdkIoBuf.h>

namespace DaqDB {

typedef Poller<DeviceTask> FinPollerQueue;

class FinalizePoller : public FinPollerQueue {
  public:
    FinalizePoller();
    virtual ~FinalizePoller() = default;

    void process() final;

  protected:
    void _processGet(DeviceTask *task);
    void _processUpdate(DeviceTask *task);
    void _processRemove(DeviceTask *task);
};

} // namespace DaqDB
