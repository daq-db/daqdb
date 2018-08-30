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

#include <cstdint>

namespace FogKV {

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
