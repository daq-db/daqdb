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

#include <functional>
#include <vector>

#include <daqdb/Types.h>

namespace DaqDB {

enum PrimaryKeyAttribute : std::int8_t {
    EMPTY = 0,
    LOCKED = (1 << 0),
    READY = (1 << 1),
    LONG_TERM = (1 << 2),
    /*
     * @TODO jradtke REMOTE flag added only for early testing purposes,
     * value location will be calculated from the key.
     */
    REMOTE = (1 << 3)
};

inline PrimaryKeyAttribute operator|(PrimaryKeyAttribute a,
                                     PrimaryKeyAttribute b) {
    return static_cast<PrimaryKeyAttribute>(static_cast<int>(a) |
                                            static_cast<int>(b));
}

struct AllocOptions {};

struct UpdateOptions {
    UpdateOptions() {}
    UpdateOptions(PrimaryKeyAttribute attr) : Attr(attr) {}

    PrimaryKeyAttribute Attr;
};

struct PutOptions {
    PutOptions() {}
    explicit PutOptions(PrimaryKeyAttribute attr) : Attr(attr) {}

    void poolerId(unsigned short id) { _poolerId = id; }

    void roundRobin(bool rr) { _roundRobin = rr; }

    unsigned short poolerId() const { return _poolerId; }

    bool roundRobin() const { return _roundRobin; }

    PrimaryKeyAttribute Attr = PrimaryKeyAttribute::EMPTY;
    unsigned short _poolerId = 0;
    bool _roundRobin = true;
};

struct GetOptions {
    GetOptions() {}
    GetOptions(PrimaryKeyAttribute attr, PrimaryKeyAttribute newAttr)
        : Attr(attr), NewAttr(newAttr) {}

    explicit GetOptions(PrimaryKeyAttribute attr) : Attr(attr), NewAttr(attr) {}

    void poolerId(unsigned short id) { _poolerId = id; }

    void roundRobin(bool rr) { _roundRobin = rr; }

    unsigned short poolerId() const { return _poolerId; }

    bool roundRobin() const { return _roundRobin; }

    PrimaryKeyAttribute Attr = PrimaryKeyAttribute::EMPTY;
    PrimaryKeyAttribute NewAttr = PrimaryKeyAttribute::EMPTY;

    unsigned short _poolerId = 0;
    bool _roundRobin = true;
};

struct KeyFieldDescriptor {
    KeyFieldDescriptor() : Size(0), IsPrimary(false) {}
    size_t Size;
    bool IsPrimary;
};

struct KeyDescriptor {
    size_t nfields() const { return _fields.size(); }

    void field(size_t idx, size_t size, bool isPrimary = false) {
        if (nfields() <= idx)
            _fields.resize(idx + 1);

        _fields[idx].Size = size;
    }

    KeyFieldDescriptor field(size_t idx) const {
        if (nfields() <= idx)
            return KeyFieldDescriptor();

        return _fields[idx];
    }

    void clear() { _fields.clear(); }

    std::vector<KeyFieldDescriptor> _fields;
};

struct OffloadOptions {
    size_t allocUnitSize = 16 * 1024;
    std::string nvmeAddr = "";
    std::string nvmeName = "";
};

struct RuntimeOptions {
    std::function<void(std::string)> logFunc = nullptr;
    std::function<void()> shutdownFunc = nullptr;
    unsigned short numOfPoolers = 1;
};

struct DhtKeyRange {
    std::string mask = "";
    std::string start = "";
    std::string end = "";
};

struct DhtNeighbor {
    std::string ip;
    unsigned short port = 0;
    DhtKeyRange keyRange;
};

struct DhtOptions {
    unsigned short port = 0;
    NodeId Id = 0;

    std::string protocol = "";
    size_t msgMaxsize = 0;
    unsigned int sccbPoolInterval = 0;
    unsigned int instantSwap = 0;

    std::vector<DhtNeighbor*> neighbors;
};

struct PMEMOptions {
    std::string poolPath = "";
    size_t totalSize = 0;
    size_t allocUnitSize = 0;
};

//---------------------------------------------------------------------------
struct ErpcOptions {
	std::string serverHostname = "";
	std::string clientHostname = "";

	uint16_t udpPort = 31850;
	uint8_t reqType = 2;
	size_t msgSize = 16;
};
//--END----------------------------------------------------------------------

struct Options {
  public:
    Options() {}
    explicit Options(const std::string &path);

    KeyDescriptor Key;
    OffloadOptions Offload;
    RuntimeOptions Runtime;
    DhtOptions Dht;

    PMEMOptions PMEM;

//---------------------------------------------------------------------------
    ErpcOptions eRPC;
//--END----------------------------------------------------------------------
};
}
