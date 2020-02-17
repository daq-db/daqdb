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

#include <functional>
#include <vector>

#include <daqdb/Types.h>

namespace DaqDB {

enum KeyValAttribute : std::int8_t {
    NOT_BUFFERED = 0,
    KVS_BUFFERED = (1 << 0),
};

enum PrimaryKeyAttribute : std::int8_t {
    EMPTY = 0,
    LOCKED = (1 << 0),
    READY = (1 << 1),
    LONG_TERM = (1 << 2),
};

enum OperationalMode : std::int8_t { STORAGE = 0, SATELLITE };

inline KeyValAttribute operator|(KeyValAttribute a, KeyValAttribute b) {
    return static_cast<KeyValAttribute>(static_cast<int>(a) |
                                        static_cast<int>(b));
}

inline PrimaryKeyAttribute operator|(PrimaryKeyAttribute a,
                                     PrimaryKeyAttribute b) {
    return static_cast<PrimaryKeyAttribute>(static_cast<int>(a) |
                                            static_cast<int>(b));
}

struct AllocOptions {
    AllocOptions() : attr(KeyValAttribute::KVS_BUFFERED) {}
    AllocOptions(KeyValAttribute attr) : attr(attr) {}

    KeyValAttribute attr;
};

struct UpdateOptions {
    UpdateOptions() : attr(PrimaryKeyAttribute::EMPTY) {}
    UpdateOptions(PrimaryKeyAttribute attr) : attr(attr) {}

    PrimaryKeyAttribute attr;
};

struct PutOptions {
    PutOptions() {}
    explicit PutOptions(PrimaryKeyAttribute attr) : attr(attr) {}

    void pollerId(unsigned short id) { _pollerId = id; }

    void roundRobin(bool rr) { _roundRobin = rr; }

    unsigned short pollerId() const { return _pollerId; }

    bool roundRobin() const { return _roundRobin; }

    PrimaryKeyAttribute attr = PrimaryKeyAttribute::EMPTY;
    unsigned short _pollerId = 0;
    bool _roundRobin = true;
};

struct GetOptions {
    GetOptions() {}
    GetOptions(PrimaryKeyAttribute attr, PrimaryKeyAttribute newAttr)
        : attr(attr), newAttr(newAttr) {}

    explicit GetOptions(PrimaryKeyAttribute attr) : attr(attr), newAttr(attr) {}

    void pollerId(unsigned short id) { _pollerId = id; }

    void roundRobin(bool rr) { _roundRobin = rr; }

    unsigned short pollerId() const { return _pollerId; }

    bool roundRobin() const { return _roundRobin; }

    PrimaryKeyAttribute attr = PrimaryKeyAttribute::EMPTY;
    PrimaryKeyAttribute newAttr = PrimaryKeyAttribute::EMPTY;

    unsigned short _pollerId = 0;
    bool _roundRobin = true;
};

struct KeyFieldDescriptor {
    KeyFieldDescriptor() : size(0), isPrimary(false) {}
    size_t size;
    bool isPrimary;
};

struct KeyDescriptor {
    size_t nfields() const { return _fields.size(); }

    size_t size() const {
        size_t size = 0;
        for (auto &f : _fields)
            size += f.size;
        return size;
    }

    void field(size_t idx, size_t size, bool isPrimary = false) {
        if (nfields() <= idx)
            _fields.resize(idx + 1);

        _fields[idx].size = size;
        _fields[idx].isPrimary = isPrimary;
    }

    KeyFieldDescriptor field(size_t idx) const {
        if (nfields() <= idx)
            return KeyFieldDescriptor();

        return _fields[idx];
    }

    void clear() { _fields.clear(); }

    std::vector<KeyFieldDescriptor> _fields;
};

enum OffloadDevType : std::int8_t { BDEV = 0, JBOD = 1, RAID0 = 2 };

struct OffloadDevDescriptor {
    OffloadDevDescriptor() = default;
    ~OffloadDevDescriptor() = default;
    std::string devName;
    std::string nvmeAddr = "";
    std::string nvmeName = "";
};

struct OffloadOptions {
    OffloadDevType devType = BDEV;
    std::string name; // Unique name
    size_t allocUnitSize =
        16 * 1024; // Allocation unit size shared across the drives in a set
    size_t raid0StripeSize = 128; // Stripe size, applicable to RAIDx only
    std::vector<OffloadDevDescriptor>
        _devs; // List of individual drives comprising the set
};

struct RuntimeOptions {
    std::function<void(std::string)> logFunc = nullptr;
    std::function<void()> shutdownFunc = nullptr;
    unsigned short baseCoreId = 0;
    unsigned short numOfPollers = 1;
    size_t maxReadyKeys = 0;
};

struct DhtKeyRange {
    std::string start = "";
    std::string end = "";
};

struct DhtNeighbor {
    std::string ip;
    unsigned short port = 0;
    bool local = false;
    DhtKeyRange keyRange;
};

struct DhtOptions {
    NodeId id = 0;
    unsigned short numOfDhtThreads = 1;
    unsigned short baseDhtId = 0;
    unsigned int maskLength = 0;
    unsigned int maskOffset = 0;
    std::vector<DhtNeighbor *> neighbors;
};

struct PMEMOptions {
    std::string poolPath = "";
    size_t totalSize = 0;
    size_t allocUnitSize = 0;
};

struct Options {
  public:
    Options() {}
    explicit Options(const std::string &path);

    DhtOptions dht;
    KeyDescriptor key;
    OperationalMode mode = OperationalMode::STORAGE;
    OffloadOptions offload;
    PMEMOptions pmem;
    RuntimeOptions runtime;
};
} // namespace DaqDB
