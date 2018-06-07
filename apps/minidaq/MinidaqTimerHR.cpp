/**
 * Copyright 2018, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of the copyright holder nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "MinidaqTimerHR.h"

namespace FogKV {

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
