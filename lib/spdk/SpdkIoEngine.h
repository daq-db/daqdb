/*
 * SpdkIoEngine.h
 *
 *  Created on: Dec 17, 2019
 *      Author: parallels
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

#include <Poller.h>
#include <Rqst.h>
#include <SpdkBdev.h>

namespace DaqDB {

class OffloadPoller;

class SpdkIoEngine : public Poller<DeviceTask> {
  public:
    SpdkIoEngine();
    virtual ~SpdkIoEngine() = default;

    void process() final;
    inline void rqstClb(const OffloadRqst *rqst, StatusCode status) {
        if (rqst->clb)
            rqst->clb(nullptr, status, rqst->key, rqst->keySize, nullptr, 0);
    }
};

} // namespace DaqDB
