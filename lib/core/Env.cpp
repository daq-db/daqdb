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

#include "Env.h"

namespace DaqDB {

const size_t DEFAULT_KEY_SIZE = 16;

Env::Env(const Options &options) : _options(options), _keySize(0) {
    for (size_t i = 0; i < options.Key.nfields(); i++)
        _keySize += options.Key.field(i).Size;
    if (_keySize)
        _keySize = DEFAULT_KEY_SIZE;
}

Env::~Env() {}

asio::io_service &Env::ioService() { return _ioService; }

const Options &Env::getOptions() { return _options; }

} // namespace DaqDB
