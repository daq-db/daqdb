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

#include <cstddef>

namespace DaqDB {

class Value {
  public:
    Value() : attr(KeyValAttribute::NOT_BUFFERED), _data(nullptr), _size(0) {}
    Value(char *data, size_t size)
        : attr(KeyValAttribute::NOT_BUFFERED), _data(data), _size(size) {}
    Value(char *data, size_t size, KeyValAttribute attr)
        : _data(data), _size(size), attr(attr) {}
    char *data() { return _data; }
    inline const char *data() const { return _data; }
    inline size_t size() const { return _size; }
    inline bool isKvsBuffered() const {
        return (attr & KeyValAttribute::KVS_BUFFERED);
    };
    inline Value &operator=(const Value &r) {
        if (&r == this)
            return *this;
        _data = r._data;
        _size = r._size;
        return *this;
    }

  protected:
    KeyValAttribute attr;
    char *_data;
    size_t _size;
};
}
