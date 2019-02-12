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

#include <chrono>

#include "MinidaqTimer.h"

namespace DaqDB {

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
