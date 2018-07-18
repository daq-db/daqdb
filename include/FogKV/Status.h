/**
 * Copyright 2017-2018, Intel Corporation
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
#pragma once

#include <cstring>
#include <limits>
#include <string>

namespace FogKV {

enum StatusCode : long {
    Ok = 0,

    _max_errno = std::numeric_limits<int>::max(),
    KeyNotFound,
    AllocationError,
    OffloadDisabledError,

    NotImplemented,
    UnknownError,
};

struct Status {

    Status() : _code(Ok) {}

    Status(int errnum) : _code(static_cast<StatusCode>(errnum)) {}

    Status(StatusCode c) : _code(c) {}

    bool ok() const { return _code == Ok; }

    StatusCode operator()() { return _code; }

    std::string to_string() {
        if (_code < _max_errno)
            return ::strerror((int)_code);

        switch (_code) {
        case NotImplemented:
            return "Not Implemented";
        default:
            return "code = " + std::to_string(_code);
        }
    }

    StatusCode _code;
};

Status errno2status(int err);

} // namespace FogKV
