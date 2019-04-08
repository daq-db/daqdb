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

#include <boost/format.hpp>
#include <iostream>

#include <chrono>

#include "DhtClient.h"
#include "DhtCore.h"

#include <Logger.h>
#include <sslot.h>

using namespace std;
using boost::format;

namespace DaqDB {

#define WAIT_FOR_NEIGHBOUR_INTERVAL 100
#define WAIT_FOR_NEIGHBOUR_RETRIES 10

/**
 * Number of times run_event_loop_once will be called waiting for response.
 * (about 1s on 2.3GHz CPU)
 */
#define WAIT_FOR_RESPONSE_IN_ERPC_LOOP_CYCLES 20 * 1000 * 1000

static void sm_handler(int, erpc::SmEventType, erpc::SmErrType, void *) {}

static void clbGet(void *ctxClient, void *ioCtx) {
    DAQ_DEBUG("Get response received");
    DhtClient *client = reinterpret_cast<DhtClient *>(ctxClient);
    DhtReqCtx *reqCtx = client->getReqCtx();

    // todo check if successful and throw otherwise
    auto *resp_msgbuf = client->getRespMsgBuf();
    auto resultMsg = reinterpret_cast<DaqdbDhtResult *>(resp_msgbuf->buf);

    reqCtx->status = resultMsg->status;
    if (resultMsg->status == StatusCode::OK) {
        auto responseSize = resultMsg->msgSize;
        reqCtx->value = new Value(new char[responseSize], responseSize);
        memcpy(reqCtx->value->data(), resultMsg->msg, responseSize);
    }

    reqCtx->ready = true;
}

static void clbGetAny(void *ctxClient, void *ioCtx) {
    DAQ_DEBUG("GetAny response received");
    DhtClient *client = reinterpret_cast<DhtClient *>(ctxClient);
    DhtReqCtx *reqCtx = client->getReqCtx();

    // todo check if successful and throw otherwise
    auto *resp_msgbuf = client->getRespMsgBuf();
    auto resultMsg = reinterpret_cast<DaqdbDhtResult *>(resp_msgbuf->buf);

    reqCtx->status = resultMsg->status;
    if (resultMsg->status == StatusCode::OK) {
        auto keySize = resultMsg->msgSize;
        reqCtx->key = new Key(new char[keySize], keySize);
        memcpy(reqCtx->key->data(), resultMsg->msg, keySize);
    }

    reqCtx->ready = true;
}

static void clbPut(void *ctxClient, void *tag) {
    DAQ_DEBUG("Put response received");
    DhtClient *client = reinterpret_cast<DhtClient *>(ctxClient);
    DhtReqCtx *reqCtx = client->getReqCtx();

    // todo check if successful and throw otherwise
    auto *resp_msgbuf = client->getRespMsgBuf();
    auto resultMsg = reinterpret_cast<DaqdbDhtResult *>(resp_msgbuf->buf);
    reqCtx->status = resultMsg->status;

    reqCtx->ready = true;
}

static void clbRemove(void *ctxClient, void *ioCtx) {
    DAQ_DEBUG("Remove response received");
    DhtClient *client = reinterpret_cast<DhtClient *>(ctxClient);
    DhtReqCtx *reqCtx = client->getReqCtx();

    // todo check if successful and throw otherwise
    auto *resp_msgbuf = client->getRespMsgBuf();
    auto resultMsg = reinterpret_cast<DaqdbDhtResult *>(resp_msgbuf->buf);
    reqCtx->status = resultMsg->status;

    reqCtx->ready = true;
}

DhtClient::DhtClient()
    : _dhtCore(nullptr), _clientRpc(nullptr), _nexus(nullptr),
      state(DhtClientState::DHT_CLIENT_INIT) {}

DhtClient::~DhtClient() {
    if (_clientRpc) {
        erpc::Rpc<erpc::CTransport> *rpc =
            reinterpret_cast<erpc::Rpc<erpc::CTransport> *>(_clientRpc);
        rpc->free_msg_buffer(*_reqMsgBuf);
        rpc->free_msg_buffer(*_respMsgBuf);
        delete reinterpret_cast<erpc::Rpc<erpc::CTransport> *>(_clientRpc);
    }
}

bool DhtClient::_initializeNode(DhtNode *node) {
    auto result = true;
    erpc::Rpc<erpc::CTransport> *rpc =
        reinterpret_cast<erpc::Rpc<erpc::CTransport> *>(_clientRpc);
    auto serverUri = boost::str(boost::format("%1%:%2%") % node->getIp() %
                                to_string(node->getPort()));
    DAQ_DEBUG("Connecting to " + serverUri);
    auto sessionNum = rpc->create_session(serverUri, 0);
    DAQ_DEBUG("Session " + std::to_string(sessionNum) + " created");

    auto numberOfRetries = WAIT_FOR_NEIGHBOUR_RETRIES;
    while (!rpc->is_connected(sessionNum) && numberOfRetries--) {
        rpc->run_event_loop(WAIT_FOR_NEIGHBOUR_INTERVAL);
    }
    if (rpc->is_connected(sessionNum)) {
        node->setSessionId(sessionNum);
    } else {
        DAQ_DEBUG("Cannot connect to: " + serverUri);
        result = false;
    }
    return result;
}

void DhtClient::initialize(DhtCore *dhtCore) {

    _dhtCore = dhtCore;
    _nexus = _dhtCore->getNexus();
    erpc::Rpc<erpc::CTransport> *rpc;

    try {
        rpc = new erpc::Rpc<erpc::CTransport>(
            _nexus, this, ++_dhtCore->numberOfClients, sm_handler);
        _clientRpc = rpc;
        _dhtCore->registerClient(this);

        auto neighbors = _dhtCore->getNeighbors();
        for (DhtNode *neighbor : *neighbors) {
            _initializeNode(neighbor);
        }
        _reqMsgBuf = std::make_unique<erpc::MsgBuffer>(
            rpc->alloc_msg_buffer_or_die(ERPC_MAX_REQUEST_SIZE));
        _respMsgBuf = std::make_unique<erpc::MsgBuffer>(
            rpc->alloc_msg_buffer_or_die(ERPC_MAX_RESPONSE_SIZE));
        state = DhtClientState::DHT_CLIENT_READY;
    } catch (exception &e) {
        DAQ_DEBUG("DHT client exception: " + std::string(e.what()));
        state = DhtClientState::DHT_CLIENT_ERROR;
        throw;
    } catch (...) {
        DAQ_DEBUG("DHT client exception: unknown");
        state = DhtClientState::DHT_CLIENT_ERROR;
        throw;
    }
}

void DhtClient::_initReqCtx() {
    _reqCtx.status = StatusCode::_MAX_ERRNO;
    _reqCtx.ready = false;
    _reqCtx.value = nullptr;
}

void DhtClient::setReqCtx(DhtReqCtx &reqCtx) { _reqCtx = reqCtx; }

void DhtClient::_runToResponse() {
    DAQ_DEBUG("Waiting for response");
    auto rpc = reinterpret_cast<erpc::Rpc<erpc::CTransport> *>(_clientRpc);

    auto timeout = WAIT_FOR_RESPONSE_IN_ERPC_LOOP_CYCLES;
    while (!_reqCtx.ready && --timeout) {
        rpc->run_event_loop_once();
    }
    if (!timeout) {
        throw OperationFailedException(Status(StatusCode::TIME_OUT));
    }
    if (_reqCtx.status != 0) {
        throw OperationFailedException(Status(_reqCtx.status));
    }
}

Value DhtClient::get(const Key &key) {
    DAQ_DEBUG("Get requested from DhtClient");
    resizeMsgBuffers(sizeof(DaqdbDhtMsg) + key.size(), ERPC_MAX_RESPONSE_SIZE);
    fillReqMsg(&key, nullptr);
    enqueueAndWait(getTargetHost(key), ErpRequestType::ERP_REQUEST_GET, clbGet);

    return *_reqCtx.value;
}

Key DhtClient::getAny() {
    DAQ_DEBUG("GetAny requested from DhtClient");
    resizeMsgBuffers(sizeof(DaqdbDhtMsg), ERPC_MAX_RESPONSE_SIZE);
    fillReqMsg(nullptr, nullptr);
    enqueueAndWait(getAnyHost(), ErpRequestType::ERP_REQUEST_GETANY, clbGetAny);

    return *_reqCtx.key;
}

void DhtClient::put(const Key &key, const Value &val) {
    resizeMsgBuffers(sizeof(DaqdbDhtMsg) + key.size() + val.size(),
                     sizeof(DaqdbDhtResult));
    fillReqMsg(&key, &val);
    enqueueAndWait(getTargetHost(key), ErpRequestType::ERP_REQUEST_PUT, clbPut);
}

void DhtClient::remove(const Key &key) {
    DAQ_DEBUG("Remove requested from DhtClient");
    resizeMsgBuffers(sizeof(DaqdbDhtMsg) + key.size(), sizeof(DaqdbDhtResult));
    fillReqMsg(&key, nullptr);
    enqueueAndWait(getTargetHost(key), ErpRequestType::ERP_REQUEST_REMOVE,
                   clbRemove);
}

bool DhtClient::ping(DhtNode &node) {
    if (state != DhtClientState::DHT_CLIENT_READY)
        throw OperationFailedException(Status(DHT_DISABLED_ERROR));

    auto rpc = reinterpret_cast<erpc::Rpc<erpc::CTransport> *>(_clientRpc);
    if (node.getSessionId() != ERPC_SESSION_NOT_SET) {
        return rpc->is_connected(node.getSessionId());
    } else {
        if (_initializeNode(&node)) {
            return rpc->is_connected(node.getSessionId());
        } else {
            return false;
        }
    }
}

Key DhtClient::allocKey(size_t keySize) {
    DAQ_DEBUG("Key alloc requested from DhtClient");
    if (_reqMsgBufInUse)
        throw OperationFailedException(Status(DHT_ALLOCATION_ERROR));
    _reqMsgBufInUse = true;
    DaqdbDhtMsg *msg = reinterpret_cast<DaqdbDhtMsg *>(_reqMsgBuf.get()->buf);
    // todo keySize is known, we don't need to send it
    // todo add size asserts
    // todo add pool
    msg->keySize = keySize;
    DAQ_DEBUG("Alloc ok");
    return Key(msg->msg, keySize, KeyValAttribute::KVS_BUFFERED);
}

void DhtClient::free(Key &&key) {
    DAQ_DEBUG("Key free requested from DhtClient");
    _reqMsgBufInUse = false;
}

erpc::MsgBuffer *DhtClient::getRespMsgBuf() { return _respMsgBuf.get(); }

Value DhtClient::alloc(const Key &key, size_t size) {
    DAQ_DEBUG("Value alloc requested from DhtClient");
    if (_reqMsgBufValInUse)
        throw OperationFailedException(Status(DHT_ALLOCATION_ERROR));
    _reqMsgBufValInUse = true;
    DaqdbDhtMsg *msg = reinterpret_cast<DaqdbDhtMsg *>(_reqMsgBuf.get()->buf);
    // todo add size asserts
    // todo add pool
    msg->valSize = size;
    DAQ_DEBUG("Alloc ok");
    return Value(msg->msg + key.size(), size, KeyValAttribute::KVS_BUFFERED);
}

void DhtClient::free(const Key &key, Value &&value) {
    DAQ_DEBUG("Value free requested from DhtClient");
    _reqMsgBufValInUse = false;
}

void DhtClient::setRpc(erpc::Rpc<erpc::CTransport> *newRpc) {
    if (_clientRpc)
        delete _clientRpc;

    _clientRpc = newRpc;
}

void DhtClient::resizeMsgBuffers(size_t new_request_size,
                                 size_t new_response_size) {
    auto rpc = reinterpret_cast<erpc::Rpc<erpc::CTransport> *>(_clientRpc);
    rpc->resize_msg_buffer(_reqMsgBuf.get(), new_request_size);
    rpc->resize_msg_buffer(_respMsgBuf.get(), new_response_size);
}

DhtNode *DhtClient::getTargetHost(const Key &key) {
    return _dhtCore->getHostForKey(key);
}

DhtNode *DhtClient::getAnyHost() { return _dhtCore->getHostAny(); }

void DhtClient::enqueueAndWait(DhtNode *targetHost, ErpRequestType type,
                               DhtContFunc contFunc) {
    auto rpc = reinterpret_cast<erpc::Rpc<erpc::CTransport> *>(_clientRpc);
    _initReqCtx();
    DAQ_DEBUG("Enqueuing request [type:" +
              to_string(static_cast<unsigned int>(type)) + "] to " +
              targetHost->getUri());
    rpc->enqueue_request(targetHost->getSessionId(),
                         static_cast<unsigned char>(type), _reqMsgBuf.get(),
                         _respMsgBuf.get(), contFunc, nullptr);
    _runToResponse();
}

void DhtClient::fillReqMsg(const Key *key, const Value *val) {
    DaqdbDhtMsg *msg = reinterpret_cast<DaqdbDhtMsg *>(_reqMsgBuf.get()->buf);
    size_t offsetMsgBuf = 0;
    if (key) {
        offsetMsgBuf = key->size();
        if (!key->isKvsBuffered()) {
            msg->keySize = key->size();
            memcpy(msg->msg, key->data(), key->size());
        }
    }
    if (val) {
        msg->valSize = val->size();
        memcpy(msg->msg + offsetMsgBuf, val->data(), msg->valSize);
    }
}

} // namespace DaqDB
