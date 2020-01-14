/**
 *  Copyright (c) 2020 Intel Corporation
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

namespace DaqDB {

class OffloadLbaAlloc {
  public:
    OffloadLbaAlloc(bool create, std::string filename, uint64_t blockCnt,
                    uint64_t avgBlkPerLba);
    ~OffloadLbaAlloc() = default;

    struct LbaBucket {
        LbaBucket() : level(0) {}
        ~LbaBucket() = default;

        static const uint32_t lbasSize = 1024;
        uint32_t sizeBracket;
        int64_t lbas[lbasSize];
        uint32_t level;

        int64_t get();
        void put(int64_t lba);
    };

    int64_t getLba(size_t ioSize);
    void putLba(int64_t lba, size_t ioSize);

    int64_t getBlock(LbaBucket &bucket, size_t ioSize);
    void putBlock(LbaBucket &bucket, size_t ioSize);

  private:
    static const uint32_t _maxBuckets = 8;
    LbaBucket buckets[_maxBuckets];

    const char *poolLayout = "queue";
    const unsigned int poolFreelistSize = 1ULL * 1024 * 1024 * 1024;
    const int pollMode = (S_IWUSR | S_IRUSR);
    pool<DaqDB::OffloadFreeList> _offloadFreeList;
    OffloadFreeList *freeLbaList = nullptr;
};

} // namespace DaqDB
