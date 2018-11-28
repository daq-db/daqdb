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

#include <boost/assign/list_of.hpp>
#include <boost/format.hpp>

#include "ip_server.h"
#include <Const.h>
#include <Util.h>

#include "DhtServer.h"

namespace DaqDB {

using boost::format;

std::map<DhtNodeState, std::string> NodeStateStr =
    boost::assign::map_list_of(DhtNodeState::NODE_READY, "Ready")(
        DhtNodeState::NODE_NOT_RESPONDING,
        "Not Responding")(DhtNodeState::NODE_INIT, "Not initialized");

DhtServer::DhtServer(asio::io_service &io_service, DhtCore *dhtCore,
                     KVStoreBase *kvs, unsigned short port)
    : state(DhtServerState::DHT_SERVER_INIT), _dhtCore(dhtCore),
      _spEpollServer(nullptr), _kvs(kvs), _thread(nullptr) {
    serve();
}

void DhtServer::_serve(void) {
    try {
        _spEpollServer.reset(new EpollServer(
            std::to_string(getPort()).c_str(),
            new IPServer(getHashMask(), _dhtCore->getRangeToHost(), _kvs)));
        state = DhtServerState::DHT_SERVER_READY;
        _spEpollServer->serve();
        state = DhtServerState::DHT_SERVER_STOPPED;
    } catch (...) {
        state = DhtServerState::DHT_SERVER_ERROR;
    }
}

void DhtServer::serve(void) {
    _thread = new std::thread(&DhtServer::_serve, this);
    do {
        sleep(1);
    } while (state == DhtServerState::DHT_SERVER_INIT);
}

Value DhtServer::get(const Key &key) {
    string lookupResult;
    auto rc = getClient()->lookup(key.data(), lookupResult);

    if (rc == 0) {
        auto size = lookupResult.size();
        auto result = Value(new char[size], size);
        std::memcpy(result.data(), lookupResult.c_str(), size);
        result.data()[result.size()] = '\0';
        return result;
    } else {
        throw OperationFailedException(Status(KEY_NOT_FOUND));
    }
}

void DhtServer::put(const Key &key, const Value &val) {
    auto rc = getClient()->insert(key.data(), val.data());
    if (rc != 0) {
        throw OperationFailedException(Status(UNKNOWN_ERROR));
    }
}

string DhtServer::printStatus() {
    std::stringstream result;

    if (_dhtCore->getClient()->isInitialized()) {
        result << "DHT: active";
    } else {
        result << "DHT: inactive";
    }

    return result.str();
}

std::string DhtServer::printNeighbors() {
    std::stringstream result;
    auto neighbors = _dhtCore->getNeighbors();
    if (neighbors->size()) {
        for (auto neighbor : *neighbors) {
            if (getClient()->ping(neighbor->getId()) ==
                zh::Const::toInt(zh::Const::ZSC_REC_SUCC)) {
                neighbor->state = DhtNodeState::NODE_READY;
            } else {
                neighbor->state = DhtNodeState::NODE_NOT_RESPONDING;
            }
            result << boost::str(
                boost::format("[%1%:%2%]: %3%\n") % neighbor->getIp() %
                to_string(neighbor->getPort()) % NodeStateStr[neighbor->state]);
        }
    } else {
        result << "No neighbors";
    }

    return result.str();
}

} // namespace DaqDB
