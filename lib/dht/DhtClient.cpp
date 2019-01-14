/**
 * Copyright 2018 - 2019 Intel Corporation.
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

#define ERPC_MAX_WAIT_TIME_S		1.0
#define ERPC_MAX_REQUEST_SIZE		16 * 1024
#define ERPC_MAX_RESPONSE_SIZE		16 * 1024

// Duration of event loop in ms
/** @todo Make it configurable */
const unsigned int DHT_CLIENT_EVENT_LOOP_MS = 10000;

static void sm_handler(int, erpc::SmEventType, erpc::SmErrType, void *) {}

static void clbGet(erpc::RespHandle *respHandle, void *ctxClient,
                   size_t ioCtx) {
    DAQ_DEBUG("Get response received");
    DhtClient *client = reinterpret_cast<DhtClient *>(ctxClient);
    DhtReqCtx *reqCtx = client->getReqCtx();
    auto rpc =
        reinterpret_cast<erpc::Rpc<erpc::CTransport> *>(client->getRpc());

    auto *resp_msgbuf = respHandle->get_resp_msgbuf();
    auto resultMsg = reinterpret_cast<DaqdbDhtResult *>(resp_msgbuf->buf);

    reqCtx->rc = resultMsg->rc;
    if (resultMsg->rc == 0) {
        auto responseSize = resultMsg->msgSize;
        reqCtx->value = new Value(new char[responseSize], responseSize);
        memcpy(reqCtx->value->data(), resultMsg->msg, responseSize);
    }

    reqCtx->ready = true;
    rpc->release_response(respHandle);
}

static void clbPut(erpc::RespHandle *respHandle, void *ctxClient,
                   size_t tag) {
    DAQ_DEBUG("Put response received");
    DhtClient *client = reinterpret_cast<DhtClient *>(ctxClient);
    DhtReqCtx *reqCtx = client->getReqCtx();
    auto rpc =
        reinterpret_cast<erpc::Rpc<erpc::CTransport> *>(client->getRpc());

    auto *resp_msgbuf = respHandle->get_resp_msgbuf();
    auto resultMsg = reinterpret_cast<DaqdbDhtResult *>(resp_msgbuf->buf);
    reqCtx->rc = resultMsg->rc;

    reqCtx->ready = true;
    rpc->release_response(respHandle);
}

static void clbRemove(erpc::RespHandle *respHandle, void *ctxClient,
                      size_t ioCtx) {
    DAQ_DEBUG("Remove response received");
    DhtClient *client = reinterpret_cast<DhtClient *>(ctxClient);
    DhtReqCtx *reqCtx = client->getReqCtx();
    auto rpc =
        reinterpret_cast<erpc::Rpc<erpc::CTransport> *>(client->getRpc());

    auto *resp_msgbuf = respHandle->get_resp_msgbuf();
    auto resultMsg = reinterpret_cast<DaqdbDhtResult *>(resp_msgbuf->buf);
    reqCtx->rc = resultMsg->rc;

    reqCtx->ready = true;
    rpc->release_response(respHandle);
}

DhtClient::DhtClient(DhtCore *dhtCore, unsigned short port)
    : _dhtCore(dhtCore), _clientRpc(nullptr), _nexus(nullptr),
      state(DhtClientState::DHT_CLIENT_INIT) {}

DhtClient::~DhtClient() {
    if (_clientRpc) {
        erpc::Rpc<erpc::CTransport> *rpc =
            reinterpret_cast<erpc::Rpc<erpc::CTransport> *>(_clientRpc);
        rpc->free_msg_buffer(*_reqMsgBuf);
        rpc->free_msg_buffer(*_respMsgBuf);
        delete reinterpret_cast<erpc::Rpc<erpc::CTransport> *>(_clientRpc);
    }
    if (_nexus)
        delete reinterpret_cast<erpc::Nexus *>(_nexus);
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

void DhtClient::initialize() {
    auto nexus =
        new erpc::Nexus(_dhtCore->getLocalNode()->getClientUri(), 0, 0);
    _nexus = nexus;
    erpc::Rpc<erpc::CTransport> *rpc;

    try {
        rpc = new erpc::Rpc<erpc::CTransport>(nexus, this, 1, sm_handler);
        _clientRpc = rpc;

        auto neighbors = _dhtCore->getNeighbors();
        for (DhtNode *neighbor : *neighbors) {
            _initializeNode(neighbor);
        }
        _reqMsgBuf = std::make_unique<erpc::MsgBuffer>(rpc->alloc_msg_buffer_or_die(ERPC_MAX_REQUEST_SIZE));
        _respMsgBuf = std::make_unique<erpc::MsgBuffer>(rpc->alloc_msg_buffer_or_die(ERPC_MAX_RESPONSE_SIZE));
        state = DhtClientState::DHT_CLIENT_READY;
    }
    catch (exception &e) {
        DAQ_DEBUG("DHT client exception: " + std::string(e.what()));
        state = DhtClientState::DHT_CLIENT_ERROR;
        throw;
    }
    catch (...) {
        DAQ_DEBUG("DHT client exception: unknown");
        state = DhtClientState::DHT_CLIENT_ERROR;
        throw;
    }
}

void DhtClient::_initReqCtx() {
    _reqCtx.rc = -1;
    _reqCtx.ready = false;
    _reqCtx.value = nullptr;
}

void DhtClient::_runToResponse() {
    DAQ_DEBUG("Waiting for response");
    auto rpc = reinterpret_cast<erpc::Rpc<erpc::CTransport> *>(_clientRpc);
    double ttime = rpc->sec_since_creation() + ERPC_MAX_WAIT_TIME_S;
    while (!_reqCtx.ready) {
        rpc->run_event_loop_once();
        if (rpc->sec_since_creation() > ttime) {
            DAQ_DEBUG("Timeout");
            throw OperationFailedException(Status(TIME_OUT));
        }
    }
    if (_reqCtx.rc != 0) {
        DAQ_DEBUG("Unknown error occurred");
        throw OperationFailedException(Status(UNKNOWN_ERROR));
    }
}

Value DhtClient::get(const Key &key) {
    auto rpc = reinterpret_cast<erpc::Rpc<erpc::CTransport> *>(_clientRpc);

    if (state != DhtClientState::DHT_CLIENT_READY)
        throw OperationFailedException(Status(DHT_DISABLED_ERROR));

    rpc->resize_msg_buffer(_reqMsgBuf.get(), sizeof(DaqdbDhtMsg) + key.size());
    rpc->resize_msg_buffer(_respMsgBuf.get(), ERPC_MAX_RESPONSE_SIZE);

    DaqdbDhtMsg *msg = reinterpret_cast<DaqdbDhtMsg *>(_reqMsgBuf.get()->buf);
    msg->keySize = key.size();
    memcpy(msg->msg, key.data(), key.size());

    auto hostToSend = _dhtCore->getHostForKey(key);
    DAQ_DEBUG("Enqueuing get request to " + hostToSend->getUri());
    _initReqCtx();
    rpc->enqueue_request(
        hostToSend->getSessionId(),
        static_cast<unsigned char>(ErpRequestType::ERP_REQUEST_GET), _reqMsgBuf.get(),
        _respMsgBuf.get(), clbGet, 0);
    _runToResponse();
    if (_reqCtx.value == nullptr) {
        DAQ_DEBUG("Key was not found");
        throw OperationFailedException(Status(KEY_NOT_FOUND));
    }

    return *_reqCtx.value;
}

void DhtClient::put(const Key &key, const Value &val) {
    auto rpc = reinterpret_cast<erpc::Rpc<erpc::CTransport> *>(_clientRpc);

    if (state != DhtClientState::DHT_CLIENT_READY)
        throw OperationFailedException(Status(DHT_DISABLED_ERROR));

    // todo add size checks
    rpc->resize_msg_buffer(_reqMsgBuf.get(), sizeof(DaqdbDhtMsg) + key.size() + val.size());
    rpc->resize_msg_buffer(_respMsgBuf.get(), sizeof(DaqdbDhtResult));

    DaqdbDhtMsg *msg = reinterpret_cast<DaqdbDhtMsg *>(_reqMsgBuf.get()->buf);
    msg->keySize = key.size();
    memcpy(msg->msg, key.data(), key.size());
    msg->valSize = val.size();
    memcpy(msg->msg + key.size(), val.data(), msg->valSize);

    auto hostToSend = _dhtCore->getHostForKey(key);
    DAQ_DEBUG("Enqueuing put request to " + hostToSend->getUri());
    _initReqCtx();
    rpc->enqueue_request(
        hostToSend->getSessionId(),
        static_cast<unsigned char>(ErpRequestType::ERP_REQUEST_PUT), _reqMsgBuf.get(),
        _respMsgBuf.get(), clbPut, 0);
    _runToResponse();
}

void DhtClient::remove(const Key &key) {
    auto rpc = reinterpret_cast<erpc::Rpc<erpc::CTransport> *>(_clientRpc);

    if (state != DhtClientState::DHT_CLIENT_READY)
        throw OperationFailedException(Status(DHT_DISABLED_ERROR));

    rpc->resize_msg_buffer(_reqMsgBuf.get(), sizeof(DaqdbDhtMsg) + key.size());
    rpc->resize_msg_buffer(_respMsgBuf.get(), sizeof(DaqdbDhtResult));

    DaqdbDhtMsg *msg = reinterpret_cast<DaqdbDhtMsg *>(_reqMsgBuf.get()->buf);
    msg->keySize = key.size();
    memcpy(msg->msg, key.data(), key.size());

    auto hostToSend = _dhtCore->getHostForKey(key);
    DAQ_DEBUG("Enqueuing remove request to " + hostToSend->getUri());
    _initReqCtx();
    rpc->enqueue_request(
        hostToSend->getSessionId(),
        static_cast<unsigned char>(ErpRequestType::ERP_REQUEST_REMOVE), _reqMsgBuf.get(),
        _respMsgBuf.get(), clbRemove, 0);
    _runToResponse();
}

bool DhtClient::ping(DhtNode &node) {
    auto rpc = reinterpret_cast<erpc::Rpc<erpc::CTransport> *>(_clientRpc);

    if (state != DhtClientState::DHT_CLIENT_READY)
        throw OperationFailedException(Status(DHT_DISABLED_ERROR));

    if (node.getSessionId() != ERPC_SESSION_NOT_SET) {
        return rpc->is_connected(node.getSessionId());
    } else {
        return false;
    }
}

} // namespace DaqDB
