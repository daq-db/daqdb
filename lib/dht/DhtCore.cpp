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

#include <boost/filesystem.hpp>

#include <rpc.h>

#include "DhtCore.h"
#include <Logger.h>

using namespace std;

namespace bf = boost::filesystem;

using namespace std;

namespace DaqDB {

const string DEFAULT_ERPC_SERVER_IP = "localhost";
const unsigned short DEFAULT_ERPC_SERVER_PORT = 31850;

const size_t DEFAULT_ERPC_NUMA_NODE = 0;
const size_t DEFAULT_ERPC_NUM_OF_THREADS = 0;

DhtCore::DhtCore(DhtOptions dhtOptions) : options(dhtOptions) {
    _initNeighbors();
    _initRangeToHost();
    _spClient.reset(new DhtClient(this, _spLocalNode->getPort()));
}

DhtCore::~DhtCore() {
    while (!_neighbors.empty()) {
        auto neighbor = _neighbors.back();
        _neighbors.pop_back();
        delete neighbor;
    }
}

void DhtCore::initClient() { _spClient->initialize(); }

void DhtCore::_initNeighbors(void) {

    for (auto option : options.neighbors) {

        auto dhtNode = new DhtNode();

        dhtNode->setIp(option->ip);
        dhtNode->setPort(option->port);
        dhtNode->state = DhtNodeState::NODE_INIT;
        dhtNode->setMaskLength(64);
        dhtNode->setMaskOffset(0);

        try {
            dhtNode->setMaskLength(option->keyRange.maskLength);
            dhtNode->setMaskOffset(option->keyRange.maskOffset);
            dhtNode->setStart(stoi(option->keyRange.start));
            dhtNode->setEnd(stoi(option->keyRange.end));
        } catch (invalid_argument &ia) {
            // no action needed
        }

        /** @todo add offset vs key length check */
        if (dhtNode->getMaskLength() > sizeof(uint64_t))
            throw OperationFailedException(Status(NOT_SUPPORTED));

        if (option->local) {
            dhtNode->state = DhtNodeState::NODE_READY;
            _spLocalNode.reset(dhtNode);
        } else {
            _neighbors.push_back(dhtNode);
        }
    }

    if (!_spLocalNode) {
        // Needed in case when local server not defined in options
        auto dhtNode = new DhtNode();
        dhtNode->setIp(DEFAULT_ERPC_SERVER_IP);
        dhtNode->setPort(DEFAULT_ERPC_SERVER_PORT);
        dhtNode->state = DhtNodeState::NODE_READY;
        dhtNode->setMaskLength(0);
        dhtNode->setMaskOffset(0);
        _spLocalNode.reset(dhtNode);
    }
}

void DhtCore::_initRangeToHost(void) {
    for (DhtNode *neighbor : _neighbors) {
        pair<int, int> key;
        key.first = neighbor->getStart();
        key.second = neighbor->getEnd();
        _rangeToHost[key] = neighbor;
    }
}

uint64_t DhtCore::_genHash(const char *key, uint64_t maskLength,
                           uint64_t maskOffset) {
    /** @todo byte-granularity assumed, do we need bit-granularity? */
    uint64_t subKey = 0;
    for (int i = 0; i < maskLength; i++)
        subKey += *reinterpret_cast<const uint8_t *>(key + maskOffset + i);
    return subKey;
}

DhtNode *DhtCore::getHostForKey(Key key) {
    if (getLocalNode()->getMaskLength() > 0) {
        auto keyHash = _genHash(key.data(), getLocalNode()->getMaskLength(),
                                getLocalNode()->getMaskOffset());
        for (auto rangeAndHost : _rangeToHost) {
            auto range = rangeAndHost.first;
            if ((keyHash >= range.first) && (keyHash <= range.second)) {
                return rangeAndHost.second;
            }
        }
    }
    return getLocalNode();
}

bool DhtCore::isLocalKey(Key key) {
    if (getLocalNode()->getMaskLength() > 0) {
        auto keyHash = _genHash(key.data(), getLocalNode()->getMaskLength(),
                                getLocalNode()->getMaskOffset());
        return (keyHash >= getLocalNode()->getStart() &&
                keyHash <= getLocalNode()->getEnd());
    } else {
        return true;
    }
}

} // namespace DaqDB
