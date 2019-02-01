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

thread_local DhtClient *DhtCore::_threadDhtClient = nullptr;

const string DEFAULT_ERPC_SERVER_IP = "localhost";
const unsigned short DEFAULT_ERPC_SERVER_PORT = 31850;

const size_t DEFAULT_ERPC_NUMA_NODE = 0;
const size_t DEFAULT_ERPC_NUM_OF_THREADS = 0;

#define DEFAULT_DHT_MASK_LENGTH 1
#define DEFAULT_DHT_MASK_OFFSET 0

DhtCore::DhtCore() : numberOfClients(0) {}

DhtCore::DhtCore(const DaqDB::DhtCore &dhtCore)
    : options(dhtCore.options), numberOfClients(0) {}

DhtCore::DhtCore(DhtOptions dhtOptions)
    : options(dhtOptions), numberOfClients(0) {
    _initNeighbors();
    _initRangeToHost();
}

DhtCore::~DhtCore() {
    while (!_neighbors.empty()) {
        auto neighbor = _neighbors.back();
        _neighbors.pop_back();
        delete neighbor;
    }
    while (!_registeredDhtClients.empty()) {
        auto dhtClient = _registeredDhtClients.back();
        _registeredDhtClients.pop_back();
        delete dhtClient;
    }
}

void DhtCore::initNexus(unsigned int port) {
    _spNexus.reset(new erpc::Nexus(getLocalNode()->getUri(port), 0, 0));
}

void DhtCore::initClient() {
    _threadDhtClient = new DhtClient();
    _threadDhtClient->initialize(this);
    if (getClient()->state == DhtClientState::DHT_CLIENT_READY) {
        DAQ_DEBUG("New DHT client started successfully");
    } else {
        DAQ_DEBUG("Can not start new DHT client");
    }
}

DhtClient *DhtCore::getClient() {
    if (!_threadDhtClient) {
        /*
         * Separate DHT client is needed per user thread.
         * It is expected that on first use the DhtClient have to be created
         * and initialized.
         */
        initClient();
    }

    return _threadDhtClient;
}

void DhtCore::setClient(DhtClient *dhtClient) { _threadDhtClient = dhtClient; }

void DhtCore::registerClient(DhtClient *dhtClient) {
    std::unique_lock<std::mutex> l(_dhtClientsMutex);
    _registeredDhtClients.push_back(dhtClient);
}

void DhtCore::_initNeighbors(void) {

    for (auto option : options.neighbors) {

        auto dhtNode = new DhtNode();

        dhtNode->setIp(option->ip);
        dhtNode->setPort(option->port);
        dhtNode->setPeerPort(option->peerPort);
        dhtNode->state = DhtNodeState::NODE_INIT;
        dhtNode->setMaskLength(DEFAULT_DHT_MASK_LENGTH);
        dhtNode->setMaskOffset(DEFAULT_DHT_MASK_OFFSET);

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
        subKey += (*reinterpret_cast<const uint8_t *>(key + maskOffset + i))
                  << (i * 8);
    return subKey;
}

DhtNode *DhtCore::getHostAny() {
    static thread_local size_t idx;
    size_t maxIdx = _neighbors.size();

    idx = ++idx % maxIdx;
    return _neighbors[idx];
}

DhtNode *DhtCore::getHostForKey(Key key) {
    if (getLocalNode()->getMaskLength() > 0) {
        auto keyHash = _genHash(key.data(), getLocalNode()->getMaskLength(),
                                getLocalNode()->getMaskOffset());
        DAQ_DEBUG("keyHash:" + std::to_string(keyHash) + " maskLen:" +
                  std::to_string(getLocalNode()->getMaskLength()) +
                  " maskOffset:" +
                  std::to_string(getLocalNode()->getMaskOffset()));
        for (auto rangeAndHost : _rangeToHost) {
            auto range = rangeAndHost.first;
            DAQ_DEBUG("Node " + rangeAndHost.second->getUri() + " serving " +
                      std::to_string(range.first) + ":" +
                      std::to_string(range.second));
            if ((keyHash >= range.first) && (keyHash <= range.second)) {
                DAQ_DEBUG("Found match at " + rangeAndHost.second->getUri());
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
