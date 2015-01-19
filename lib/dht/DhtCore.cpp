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

#include <boost/filesystem.hpp>

#include <rpc.h>

#include "DhtCore.h"
#include <Logger.h>

using namespace std;

namespace bf = boost::filesystem;

namespace DaqDB {

thread_local DhtClient *DhtCore::_threadDhtClient = nullptr;

const string DEFAULT_ERPC_SERVER_IP = "localhost";
const unsigned short DEFAULT_ERPC_SERVER_PORT = 31851;
const unsigned short DEFAULT_ERPC_PEER_PORT = 31850;

DhtCore::DhtCore() : numberOfClients(0), numberOfClientThreads(0) {}

DhtCore::DhtCore(const DaqDB::DhtCore &dhtCore)
    : options(dhtCore.options), numberOfClients(0), numberOfClientThreads(0) {}

DhtCore::DhtCore(DhtOptions dhtOptions)
    : options(dhtOptions), numberOfClients(0), numberOfClientThreads(0),
      _maskLength(dhtOptions.maskLength), _maskOffset(dhtOptions.maskOffset) {

    /** @todo add offset vs key length check */
    if (_maskLength > sizeof(uint64_t))
        throw OperationFailedException(Status(NOT_SUPPORTED));

    _initSeed();
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

void DhtCore::_initSeed() {
    auto now = std::chrono::system_clock::now();
    auto now_us = std::chrono::time_point_cast<std::chrono::microseconds>(now);
    auto us = now.time_since_epoch();
    randomSeed = us.count();
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

        try {
            dhtNode->setStart(stoi(option->keyRange.start));
            dhtNode->setEnd(stoi(option->keyRange.end));
        } catch (invalid_argument &ia) {
            // no action needed
        }

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
        dhtNode->setPeerPort(DEFAULT_ERPC_PEER_PORT);
        dhtNode->state = DhtNodeState::NODE_READY;
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

uint64_t DhtCore::_genHash(const char *key) {
    /** @todo byte-granularity assumed, do we need bit-granularity? */
    uint64_t subKey = 0;
    for (int i = 0; i < _maskLength; i++)
        subKey += (*reinterpret_cast<const uint8_t *>(key + _maskOffset + i))
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
    DAQ_DEBUG("maskLen:" + std::to_string(_maskLength) +
              " maskOffset:" + std::to_string(_maskOffset));
    if (_maskLength <= 0)
        throw OperationFailedException(Status(KEY_NOT_FOUND));
    auto keyHash = _genHash(key.data());
    DAQ_DEBUG("keyHash:" + std::to_string(keyHash));
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

    throw OperationFailedException(Status(KEY_NOT_FOUND));
}

bool DhtCore::isLocalKey(Key key) {
    if (_maskLength > 0) {
        auto keyHash = _genHash(key.data());
        return (keyHash >= getLocalNode()->getStart() &&
                keyHash <= getLocalNode()->getEnd());
    } else {
        return true;
    }
}

} // namespace DaqDB
