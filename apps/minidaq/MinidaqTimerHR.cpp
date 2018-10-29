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
