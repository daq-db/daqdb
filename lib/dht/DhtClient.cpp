/**
 * Copyright 2018-2019 Intel Corporation.
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

#include <boost/format.hpp>
#include <iostream>

#include <chrono>

#include "DhtClient.h"
#include "DhtCore.h"

#include <Logger.h>
#include <rpc.h>
#include <sslot.h>

using namespace std;
using boost::format;

namespace DaqDB {

#define ERPC_MAX_REQUEST_SIZE 32 * 1024
#define ERPC_MAX_RESPONSE_SIZE 32 * 1024

static void sm_handler(int, erpc::SmEventType, erpc::SmErrType, void *) {}

static void clbGet(void *ctxClient, size_t ioCtx) {
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

static void clbPut(void *ctxClient, size_t tag) {
    DAQ_DEBUG("Put response received");
    DhtClient *client = reinterpret_cast<DhtClient *>(ctxClient);
    DhtReqCtx *reqCtx = client->getReqCtx();

    // todo check if successful and throw otherwise
    auto *resp_msgbuf = client->getRespMsgBuf();
    auto resultMsg = reinterpret_cast<DaqdbDhtResult *>(resp_msgbuf->buf);
    reqCtx->status = resultMsg->status;

    reqCtx->ready = true;
}

static void clbRemove(void *ctxClient, size_t ioCtx) {
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

void DhtClient::_initializeNode(DhtNode *node) {
    erpc::Rpc<erpc::CTransport> *rpc =
        reinterpret_cast<erpc::Rpc<erpc::CTransport> *>(_clientRpc);
    auto serverUri = boost::str(boost::format("%1%:%2%") % node->getIp() %
                                to_string(node->getPort()));
    DAQ_DEBUG("Connecting to " + serverUri);
    auto sessionNum = rpc->create_session(serverUri, 0);
    DAQ_DEBUG("Session " + std::to_string(sessionNum) + " created");
    while (!rpc->is_connected(sessionNum))
        rpc->run_event_loop_once();
    node->setSessionId(sessionNum);
    DAQ_DEBUG("Connected!");
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
    while (!_reqCtx.ready)
        rpc->run_event_loop_once();
    if (_reqCtx.status != 0) {
        throw OperationFailedException(Status(_reqCtx.status));
    }
}

Value DhtClient::get(const Key &key) {
    DAQ_DEBUG("Get requested from DhtClient");
    // @TODO jradtke verify why communication is broken when _reqMsgBuf is
    // smaller than response size
    resizeMsgBuffers(ERPC_MAX_REQUEST_SIZE, ERPC_MAX_RESPONSE_SIZE);
    fillReqMsg(&key, nullptr);
    enqueueAndWait(getTargetHost(key), ErpRequestType::ERP_REQUEST_GET, clbGet);

    return *_reqCtx.value;
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
        return false;
    }
}

Key DhtClient::allocKey(size_t keySize) {
    DAQ_DEBUG("Key alloc requested from DhtClient");
    if (_reqMsgBufInUse)
        throw OperationFailedException(Status(ALLOCATION_ERROR));
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
        throw OperationFailedException(Status(ALLOCATION_ERROR));
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

void DhtClient::setRpc(void *newRpc) {
    if (_clientRpc) {
        delete reinterpret_cast<erpc::Rpc<erpc::CTransport> *>(_clientRpc);
    }
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

void DhtClient::enqueueAndWait(DhtNode *targetHost, ErpRequestType type,
                               DhtContFunc contFunc) {
    auto rpc = reinterpret_cast<erpc::Rpc<erpc::CTransport> *>(_clientRpc);
    _initReqCtx();
    DAQ_DEBUG("Enqueuing request [type:" +
              to_string(static_cast<unsigned int>(type)) + "] to " +
              targetHost->getUri());
    rpc->enqueue_request(targetHost->getSessionId(),
                         static_cast<unsigned char>(type), _reqMsgBuf.get(),
                         _respMsgBuf.get(), contFunc, 0);
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
