/**
 * Copyright 2017-2019 Intel Corporation.
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

#include <cstddef>

#include <daqdb/Options.h>

namespace DaqDB {

class Key {
  public:
    Key() : attr(KeyValAttribute::NOT_BUFFERED), _data(nullptr), _size(0) {}
    Key(char *data, size_t size)
        : _data(data), _size(size), attr(KeyValAttribute::NOT_BUFFERED) {}
    Key(char *data, size_t size, KeyValAttribute attr)
        : _data(data), _size(size), attr(attr) {}
    char *data() { return _data; }

    /**
     * @return true if allocated inside KVS
     */
    inline bool isKvsBuffered() const {
        return (attr & KeyValAttribute::KVS_BUFFERED);
    };

    inline const char *data() const { return _data; }
    inline size_t size() const { return _size; }

  protected:
    KeyValAttribute attr;
    char *_data;
    size_t _size;
};

} // namespace DaqDB
