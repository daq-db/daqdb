/**
 * Copyright 2018 Intel Corporation.
 *
 * This software and the related documents are Intel copyrighted materials,
 * and your use of them is governed by the express license under which they
 * were provided to you (Intel OBL Internal Use License).
 * Unless the License provides otherwise, you may not use, modify, copy,
 * publish, distribute, disclose or transmit this software or the related
 * documents without Intel's prior written permission.
 *
 * This software and the related documents are provided as is, with no
 * express or implied warranties, other than those that are expressly
 * stated in the License.
 */

#pragma once

#include <chrono>

#include "MinidaqTimer.h"

namespace FogKV {

class MinidaqTimerHR : public MinidaqTimer {
  public:
    MinidaqTimerHR();
    ~MinidaqTimerHR();

    void Restart_s(int interval_s);
    void Restart_ms(int interval_ms);
    void Restart_us(int interval_us);
    bool IsExpired();
    uint64_t GetElapsed_ns();

  private:
    std::chrono::nanoseconds _reqInterval;
    std::chrono::nanoseconds _currInterval;
    std::chrono::time_point<std::chrono::high_resolution_clock> _start;
    bool _expired = true;

    void _restart();
};
}
