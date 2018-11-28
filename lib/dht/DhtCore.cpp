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

#include <ConfHandler.h>
#include <Util.h>

#include "DhtCore.h"
#include <ConfHandler.h>
#include <Logger.h>

using namespace std;

namespace bf = boost::filesystem;
namespace zh = iit::datasys::zht::dm;

namespace DaqDB {

DhtCore::DhtCore(DhtOptions dhtOptions) : options(dhtOptions) {
    createConfFile();
    createNeighborsFile();
    zh::ConfHandler::initConf(DEFAULT_ZHT_CONF_FILE,
                              DEFAULT_ZHT_NEIGHBORS_FILE);
    _initNeighbors();
    _initRangeToHost();

    removeConfFile();
    removeNeighborsFile();

    _zhtClient.c.init("", "", getLocalNode()->getMask(), &_rangeToHost);
    _zhtClient.setInitialized();
}

void DhtCore::createConfFile(void) {

    if (!bf::exists(DEFAULT_ZHT_CONF_FILE)) {

        ofstream confOut(DEFAULT_ZHT_CONF_FILE, ios::out);

        confOut << "PROTOCOL TCP" << endl;
        confOut << "PORT " << options.port << endl;
        confOut << "MSG_MAXSIZE " << DEFAULT_ZHT_MSG_MAXSIZE << endl;
        confOut << "SCCB_POLL_INTERVAL " << DEFAULT_ZHT_SCCB_POOL_INTERVAL
                << endl;
        confOut << "INSTANT_SWAP " << DEFAULT_ZHT_INSTANT_SWAP << endl;
        confOut.close();

        DAQ_DEBUG("ZHT configuration file created");
    } else {
        DAQ_DEBUG("ZHT configuration file creation skipped");
    }
}

void DhtCore::createNeighborsFile(void) {
    if (!bf::exists(DEFAULT_ZHT_NEIGHBORS_FILE)) {
        ofstream neighbourOut(DEFAULT_ZHT_NEIGHBORS_FILE, ios::out);

        for (auto neighbor : options.neighbors) {
            neighbourOut << neighbor->ip << " " << neighbor->port << endl;
        }
        neighbourOut.close();
        DAQ_DEBUG("ZHT neighbor file created");
    } else {
        DAQ_DEBUG("ZHT neighbor file creation failed");
    }
}

void DhtCore::removeConfFile(void) {
    if (bf::exists(DEFAULT_ZHT_CONF_FILE)) {
        bf::remove(DEFAULT_ZHT_CONF_FILE);
    }
}

void DhtCore::removeNeighborsFile(void) {
    if (bf::exists(DEFAULT_ZHT_NEIGHBORS_FILE)) {
        bf::remove(DEFAULT_ZHT_NEIGHBORS_FILE);
    }
}

bool DhtCore::_isLocalServerNode(string ip, string port,
                                 unsigned short serverPort) {
    try {
        if ((ip.compare("localhost") == 0) && (std::stoi(port) == serverPort)) {
            return true;
        }
    } catch (std::invalid_argument &ia) {
        // no action needed
    }
    return false;
}

void DhtCore::_initNeighbors(void) {
    for (unsigned int index = 0; index < zh::ConfHandler::NeighborVector.size();
         ++index) {
        auto entry = zh::ConfHandler::NeighborVector.at(index);
        auto dhtNode = new DhtNode();
        dhtNode->setId(index);
        dhtNode->setIp(entry.name());
        dhtNode->setPort(std::stoi(entry.value()));
        dhtNode->state = DhtNodeState::NODE_INIT;

        for (auto option : options.neighbors) {
            if (option->ip == dhtNode->getIp() &&
                option->port == dhtNode->getPort()) {
                try {
                    dhtNode->setMask(std::stoi(option->keyRange.mask));
                    dhtNode->setStart(std::stoi(option->keyRange.start));
                    dhtNode->setEnd(std::stoi(option->keyRange.end));
                } catch (std::invalid_argument &ia) {
                    // no action needed
                }
            }
        }
        if (_isLocalServerNode(entry.name(), entry.value(), options.port)) {
            _spLocalNode.reset(dhtNode);
        } else {
            _neighbors.push_back(dhtNode);
        }
    }
}

void DhtCore::_initRangeToHost(void) {
    for (DhtNode *neighbor : _neighbors) {
        std::pair<int, int> key;
        key.first = neighbor->getStart();
        key.second = neighbor->getEnd();
        _rangeToHost[key] = neighbor->getId();
    }
}

uint64_t DhtCore::_genHash(const string &key, int mask) {
    uint64_t hash = 0;
    uint64_t c; // int c;

    auto maskCount = mask;
    auto pc = key.c_str();
    while ((c = (*pc++)) && (--maskCount >= 0)) {
        hash += c << maskCount;
    }

    return hash;
}

bool DhtCore::isLocalKey(Key key) {
    if (getLocalNode()->getMask() > 0) {
        auto keyHash = _genHash(key.data(), getLocalNode()->getMask());
        return (keyHash >= getLocalNode()->getStart() &&
                keyHash < getLocalNode()->getEnd());
    } else {
        return true;
    }
}

} // namespace DaqDB
