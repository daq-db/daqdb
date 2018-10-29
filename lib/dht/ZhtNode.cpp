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

#include <sstream>

#include <boost/algorithm/string.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>

#include <ConfHandler.h>
#include <Util.h>

#include "DhtUtils.h"
#include "ZhtNode.h"
#include <Logger.h>

#include "EpollServer.h"
#include "ip_server.h"

namespace DaqDB {

using zht_const = iit::datasys::zht::dm::Const;
using boost::format;

std::map<NodeState, std::string> NodeStateStr = boost::assign::map_list_of(
    NodeState::Ready, "Ready")(NodeState::NotResponding, "Not Responding");

ZhtNode::ZhtNode(asio::io_service &io_service, DaqDB::Env *env,
                 unsigned short port)
    : DaqDB::DhtNode(io_service, port), _env(env),
      _confFile(env->getZhtConfFile()),
      _neighborsFile(env->getZhtNeighborsFile()) {
    setPort(DaqDB::utils::getFreePort(io_service, port, true));

    _initZhtConf();
    _thread = new std::thread(&ZhtNode::_ThreadMain, this);
}

ZhtNode::~ZhtNode() {
    for (auto neighbor : _neighbors) {
        delete neighbor.first;
        delete neighbor.second;
    }
}

void ZhtNode::_initZhtConf() {
    _env->createZhtConfFiles();
    _env->createSpdkConfFiles();
    ConfHandler::initConf(_confFile, _neighborsFile);
    _env->removeSpdkConfFiles();
    _env->removeZhtConfFiles();
}

void ZhtNode::_ThreadMain() {
    auto zhtPort = to_string(getPort());
    _initNeighbors();

    int hash_mask = 0;
    std::map<std::pair<int, int>, int> rangeToHost;
    std::pair<int, int> local_key;
    for (auto neighbor : _neighbors) {
        std::pair<int, int> key;
        key.first = neighbor.second->start;
        key.second = neighbor.second->end;
        // @TODO this value should be calculated differently
        hash_mask = neighbor.second->mask;
        rangeToHost[key] = neighbor.first->getDhtId();
    }

    EpollServer zhtServer(zhtPort.c_str(),
                          new IPServer(hash_mask, rangeToHost, _env->getKvs()));
    _client.c.init(_confFile, _neighborsFile, hash_mask, rangeToHost);
    _client.setInitialized();

    zhtServer.serve();
}

void ZhtNode::_initNeighbors() {
    for (unsigned int index = 0; index < ConfHandler::NeighborVector.size();
         ++index) {
        auto entry = ConfHandler::NeighborVector.at(index);
        auto dhtNode =
            new PureNode(entry.name(), index, std::stoi(entry.value()));
        auto dhtNodeInfo = new DhtNodeInfo();
        dhtNodeInfo->state = NodeState::Unknown;
        for (auto option : _env->getOptions().Dht.neighbors) {
            if (option->ip == dhtNode->getIp() &&
                option->port == dhtNode->getPort()) {
                try {
                    dhtNodeInfo->mask = std::stoi(option->keyRange.mask);
                    dhtNodeInfo->start = std::stoi(option->keyRange.start);
                    dhtNodeInfo->end = std::stoi(option->keyRange.end);
                } catch (std::invalid_argument &ia) {
                    // no action needed
                }
            }
        }
        _neighbors[dhtNode] = dhtNodeInfo;
    }
}

Value ZhtNode::Get(const Key &key) {
    string lookupResult;
    auto rc = _client.c.lookup(key.data(), lookupResult);

    if (rc == 0) {
        auto size = lookupResult.size();
        auto result = Value(new char[size], size);
        std::memcpy(result.data(), lookupResult.c_str(), size);
        result.data()[result.size()] = '\0';
        return result;
    } else {
        throw OperationFailedException(Status(KeyNotFound));
    }
}

void ZhtNode::Put(const Key &key, const Value &val) {
    auto rc = _client.c.insert(key.data(), val.data());
    if (rc != 0) {
        throw OperationFailedException(Status(UnknownError));
    }
}

std::string ZhtNode::printStatus() {
    std::stringstream result;

    if (_client.isInitialized()) {
        result << "DHT: active";
    } else {
        result << "DHT: inactive";
    }

    return result.str();
}

std::string ZhtNode::printNeighbors() {
    std::stringstream result;

    if (_neighbors.size()) {
        for (auto neighbor : _neighbors) {

            if (_client.c.ping(neighbor.first->getDhtId()) ==
                zht_const::toInt(zht_const::ZSC_REC_SUCC)) {
                neighbor.second->state = NodeState::Ready;
            } else {
                neighbor.second->state = NodeState::NotResponding;
            }
            result << boost::str(boost::format("[%1%:%2%]: %3%\n") %
                                 neighbor.first->getIp() %
                                 to_string(neighbor.first->getPort()) %
                                 NodeStateStr[neighbor.second->state]);
        }
    } else {
        result << "No neighbors";
    }

    return result.str();
}

} // namespace DaqDB
