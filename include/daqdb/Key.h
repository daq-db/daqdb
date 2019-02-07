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
