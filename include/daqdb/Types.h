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

#include <stdexcept>

namespace DaqDB {

typedef unsigned int NodeId;

class NotImplementedException : public std::logic_error {
  public:
    explicit NotImplementedException(const std::string &what)
        : logic_error("Not Implemented: " + what) {}

    NotImplementedException() : logic_error("Not Implemented") {}
};

class OperationFailedException : public std::runtime_error {
  public:
    OperationFailedException(Status s, const std::string &msg = "")
        : runtime_error(msg), _status(s) {
        set_what();
    }
    explicit OperationFailedException(const std::string &msg = "")
        : runtime_error(msg), _status(UnknownError) {
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

}
