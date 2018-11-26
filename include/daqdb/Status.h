/**
 * Copyright 2017-2018 Intel Corporation.
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

#include <cstring>
#include <limits>
#include <string>

namespace DaqDB {

enum StatusCode : long {
    OK = 0,

    _MAX_ERRNO = std::numeric_limits<int>::max(),
    KEY_NOT_FOUND,
    ALLOCATION_ERROR,
    OFFLOAD_DISABLED_ERROR,
    TIME_OUT,
    NOT_IMPLEMENTED,
    UNKNOWN_ERROR,
};

struct Status {

    Status() : _code(OK) {}

    Status(long errnum) : _code(static_cast<StatusCode>(errnum)) {}
    explicit Status(StatusCode c) : _code(c) {}

    bool ok() const { return _code == OK; }

    StatusCode operator()() { return _code; }

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

    StatusCode _code;
};

Status errno2status(int err);

}
