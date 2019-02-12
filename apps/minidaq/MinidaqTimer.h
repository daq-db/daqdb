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

#include <cstdint>

namespace DaqDB {

class MinidaqTimer {
  public:
    MinidaqTimer();
    virtual ~MinidaqTimer();

    virtual void Restart_s(int interval_s) = 0;
    virtual void Restart_ms(int interval_ms) = 0;
    virtual void Restart_us(int interval_us) = 0;
    virtual bool IsExpired() = 0;
    virtual uint64_t GetElapsed_ns() = 0;
};
}
