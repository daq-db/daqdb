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

#include "MinidaqTimerHR.h"

namespace DaqDB {

MinidaqTimerHR::MinidaqTimerHR() {}

MinidaqTimerHR::~MinidaqTimerHR() {}

void MinidaqTimerHR::_restart() {
    _expired = false;
    _start = std::chrono::high_resolution_clock::now();
}

void MinidaqTimerHR::Restart_s(int interval_s) {
    _reqInterval = std::chrono::seconds(interval_s);
    _restart();
}

void MinidaqTimerHR::Restart_ms(int interval_ms) {
    _reqInterval = std::chrono::milliseconds(interval_ms);
    _restart();
}

void MinidaqTimerHR::Restart_us(int interval_us) {
    _reqInterval = std::chrono::microseconds(interval_us);
    _restart();
}

bool MinidaqTimerHR::IsExpired() {
    if (_expired) {
        return _expired;
    }
    auto now = std::chrono::high_resolution_clock::now();
    _currInterval =
        std::chrono::duration_cast<std::chrono::nanoseconds>(now - _start);
    if (_currInterval >= _reqInterval) {
        _expired = true;
    }
    return _expired;
}

uint64_t MinidaqTimerHR::GetElapsed_ns() {
    IsExpired();
    std::chrono::duration<uint64_t, std::nano> ns(_currInterval);
    return ns.count();
}
}
