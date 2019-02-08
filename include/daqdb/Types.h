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

#include <stdexcept>

#include "Status.h"

namespace DaqDB {

typedef unsigned int NodeId;

#define FUNC_NOT_IMPLEMENTED                                                   \
    NotImplementedException("function: " + std::string(__func__) + " [" +      \
                            std::string(__FILE__) + ":" +                      \
                            std::to_string(__LINE__) + "]")

#define FUNC_NOT_SUPPORTED                                                     \
    NotSupportedFuncException("function: " + std::string(__func__) + " [" +    \
                              std::string(__FILE__) + ":" +                    \
                              std::to_string(__LINE__) + "]")

class NotImplementedException : public std::logic_error {
  public:
    explicit NotImplementedException(const std::string &what)
        : logic_error("Not Implemented: " + what) {}

    NotImplementedException() : logic_error("Not Implemented") {}
};

class NotSupportedFuncException : public std::logic_error {
  public:
    explicit NotSupportedFuncException(const std::string &what)
        : logic_error("Not Supported: " + what) {}

    NotSupportedFuncException() : logic_error("Not Supported") {}
};

class OperationFailedException : public std::runtime_error {
  public:
    OperationFailedException(Status s, const std::string &msg = "")
        : runtime_error(msg), _status(s) {
        set_what();
    }
    explicit OperationFailedException(const std::string &msg = "")
        : runtime_error(msg), _status(UNKNOWN_ERROR) {
        set_what();
    }
    OperationFailedException(int errnum, const std::string &msg = "")
        : runtime_error(msg), _status(errnum) {
        set_what();
    }

    virtual const char *what() const noexcept { return _what.c_str(); }

    Status status() { return _status; }
    Status _status;
    std::string _what;

  private:
    void set_what() {
        _what = runtime_error::what();

        if (!_status.ok() &&
            _what.find(_status.to_string()) == std::string::npos) {
            _what += ": " + _status.to_string();
        }
    }
};

class QueueFullException : public std::runtime_error {
  public:
    QueueFullException() : runtime_error("Queue full") {}
};

} // namespace DaqDB
