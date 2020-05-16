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

#define DAQDB_DEFAULT_ERROR_MSG "DAQDB Error: "

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
    Status() : _code(OK),_errorMsg(DAQDB_DEFAULT_ERROR_MSG) {}

    Status(long errnum) : _code(static_cast<StatusCode>(errnum)), _errorMsg(DAQDB_DEFAULT_ERROR_MSG) {}
    explicit Status(StatusCode c) : _code(c), _errorMsg(DAQDB_DEFAULT_ERROR_MSG) {}

    bool ok() const { return _code == OK; }

    StatusCode operator()() { return _code; }

    StatusCode getStatusCode() { return _code; }

    std::string to_string() {

        if (_code < _MAX_ERRNO)
            return ::strerror((int)_code);
        switch (_code) {
        case KEY_NOT_FOUND:
            return _errorMsg + "KEY_NOT_FOUND";
        case BAD_KEY_FORMAT:
            return _errorMsg + "BAD_KEY_FORMAT";
        case PMEM_ALLOCATION_ERROR:
            return _errorMsg + "PMEM_ALLOCATION_ERROR";
        case DHT_ALLOCATION_ERROR:
            return _errorMsg + "DHT_ALLOCATION_ERROR";
        case  SPDK_ALLOCATION_ERROR:
            return _errorMsg + "SPDK_ALLOCATION_ERROR";
        case OFFLOAD_DISABLED_ERROR:
            return _errorMsg + "OFFLOAD_DISABLED_ERROR";
        case QUEUE_FULL_ERROR:
            return _errorMsg + "QUEUE_FULL_ERROR";
        case DHT_DISABLED_ERROR:
            return _errorMsg + "DHT_DISABLED_ERROR";
        case TIME_OUT:
            return _errorMsg + "TIME_OUT";
        case DHT_CONNECT_ERROR:
            return _errorMsg + "DHT_CONNECT_ERROR_STATUSAAA";
        case NOT_IMPLEMENTED:
            return _errorMsg + "NOT_IMPLEMENTED";
        case NOT_SUPPORTED:
            return _errorMsg + "NOT_SUPPORTED";
        case UNKNOWN_ERROR:
            return _errorMsg + "UNKNOWN_ERROR";
        default:
            return "code = " + std::to_string(_code);
        }
    }

  private:
    StatusCode _code;
    std::string _errorMsg;
};

Status errno2status(int err);

} // namespace DaqDB
