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

#include <iostream>
#include <string>

#include "OffloadFreeList.h"
#include "OffloadLbaAlloc.h"

namespace DaqDB {

OffloadLbaAlloc::OffloadLbaAlloc(bool create, std::string filename,
                                 uint64_t blockCnt, uint64_t avgBlkPerLba) {
    bool initNeeded = false;

    if (create == false) {
        _offloadFreeList =
            pool<OffloadFreeList>::open(filename.c_str(), poolLayout);
    } else {
        _offloadFreeList = pool<OffloadFreeList>::create(
            filename.c_str(), poolLayout, poolFreelistSize, pollMode);
        initNeeded = true;
    }
    freeLbaList = _offloadFreeList.get_root().get();
    freeLbaList->maxLba = blockCnt / avgBlkPerLba;
    if (initNeeded) {
        freeLbaList->push(_offloadFreeList, -1);
        for (uint32_t slot = 0; slot < numSlots; slot++) {
            freeLbaList->pushBlock(_offloadFreeList, -1, slot);
        }
    }

    for (uint32_t i = 0; i < _maxBuckets; i++) {
        buckets[i].level = 0;
        buckets[i].sizeBracket = i + 1; // 4kB, 8kB, 12kB, ...
    }
}

int64_t OffloadLbaAlloc::getLba(size_t ioSize) {
    uint32_t bucketNum = ioSize / 4096;
    if (bucketNum >= _maxBuckets) {
        return -1;
    }
    if (!buckets[bucketNum].level) {
        int64_t ret = getBlock(buckets[bucketNum], bucketNum);
        if (ret < 0) {
            return ret;
        }
    }
    return buckets[bucketNum].get();
}

int64_t OffloadLbaAlloc::getBlock(LbaBucket &bucket, size_t ioSize) {
    int64_t ret = freeLbaList->getBlock(_offloadFreeList, bucket.lbas,
                                        bucket.lbasSize, ioSize);
    if (ret < 0) {
        return ret;
    }
    bucket.level = static_cast<uint32_t>(ret);
    return ret;
}

void OffloadLbaAlloc::putLba(int64_t lba, size_t ioSize) {
    uint32_t bucketNum = ioSize / 4096;
    if (bucketNum >= _maxBuckets) {
        return;
    }
    if (buckets[bucketNum].level >= buckets[bucketNum].lbasSize) {
        putBlock(buckets[bucketNum], bucketNum);
    }
    buckets[bucketNum].put(lba);
}

void OffloadLbaAlloc::putBlock(LbaBucket &bucket, size_t ioSize) {
    freeLbaList->putBlock(_offloadFreeList, bucket.lbas, bucket.lbasSize,
                          ioSize);
    bucket.level = 0;
}

int64_t OffloadLbaAlloc::LbaBucket::get() {
    if (!level)
        return -1;
    int64_t lba = lbas[level - 1];
    level--;
    return lba;
}

void OffloadLbaAlloc::LbaBucket::put(int64_t lba) {
    if (level >= lbasSize)
        return;
    level++;
    lbas[level] = lba;
    return;
}

} // namespace DaqDB
