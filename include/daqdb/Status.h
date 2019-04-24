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

#include <cstring>
#include <limits>
#include <string>

namespace DaqDB {

enum StatusCode : long {
    OK = 0,

    _MAX_ERRNO = std::numeric_limits<int>::max(),
    KEY_NOT_FOUND,
    BAD_KEY_FORMAT,
    PMEM_ALLOCATION_ERROR,
    DHT_ALLOCATION_ERROR,
    SPDK_ALLOCATION_ERROR,
    OFFLOAD_DISABLED_ERROR,
    QUEUE_FULL_ERROR,
    DHT_DISABLED_ERROR,
    TIME_OUT,
    DHT_CONNECT_ERROR,
    NOT_IMPLEMENTED,
    NOT_SUPPORTED,
    UNKNOWN_ERROR,
};

class Status {
  public:
    Status() : _code(OK) {}

    Status(long errnum) : _code(static_cast<StatusCode>(errnum)) {}
    explicit Status(StatusCode c) : _code(c) {}

    bool ok() const { return _code == OK; }

    StatusCode operator()() { return _code; }

    StatusCode getStatusCode() { return _code; }

    std::string to_string() {
        if (_code < _MAX_ERRNO)
            return ::strerror((int)_code);

        switch (_code) {
        case NOT_IMPLEMENTED:
            return "Not Implemented";
        default:
            return "code = " + std::to_string(_code);
        }
    }

  private:
    StatusCode _code;
};

Status errno2status(int err);

} // namespace DaqDB
