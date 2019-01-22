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

#include <cstddef>

#include <daqdb/Options.h>

namespace DaqDB {

class Key {
  public:
    Key() : attr(KeyAttribute::NOT_BUFFERED), _data(nullptr), _size(0) {}
    Key(char *data, size_t size)
        : _data(data), _size(size), attr(KeyAttribute::NOT_BUFFERED) {}
    Key(char *data, size_t size, KeyAttribute attr)
        : _data(data), _size(size), attr(attr) {}
    char *data() { return _data; }

    /**
     * @return true if allocated inside DHT buffer
     */
    inline bool isDhtBuffered() const {
        return (attr & KeyAttribute::DHT_BUFFERED);
    }

    inline const char *data() const { return _data; }
    inline size_t size() const { return _size; }

  protected:
    KeyAttribute attr;
    char *_data;
    size_t _size;
};

} // namespace DaqDB
