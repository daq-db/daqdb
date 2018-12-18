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

#include <boost/format.hpp>
#include <iostream>

#include <chrono>

#include "DhtClient.h"
#include "DhtCore.h"

#include <rpc.h>
#include <sslot.h>

using namespace std;
using boost::format;

namespace DaqDB {

const size_t DEFAULT_ERPC_REQ_SIZE = 16;
const size_t DEFAULT_ERPC_RESPONSE_PREALOC_SIZE = 16;
const size_t DEFAULT_ERPC_RESPONSE_BUF_SIZE = 16 * 1024;
const size_t DEFAULT_ERPC_REQEST_BUF_SIZE = 16 * 1024;

const unsigned int DHT_CLIENT_POLL_INTERVAL = 100;

static void sm_handler(int, erpc::SmEventType, erpc::SmErrType, void *) {}

static void clbGet(erpc::RespHandle *respHandle, void *ctxClient,
                   size_t ioCtx) {
    DhtClient *client = reinterpret_cast<DhtClient *>(ctxClient);
    DhtReqCtx *reqCtx = reinterpret_cast<DhtReqCtx *>(ioCtx);
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
    reqCtx->cv.notify_all();

    // TODO: jradtke Performance optimization possible if resp_msgbuf reused
    rpc->release_response(respHandle);
}

static void clbPut(erpc::RespHandle *respHandle, void *ctxClient,
                   size_t ioCtx) {
    DhtClient *client = reinterpret_cast<DhtClient *>(ctxClient);
    DhtReqCtx *reqCtx = reinterpret_cast<DhtReqCtx *>(ioCtx);
    auto rpc =
        reinterpret_cast<erpc::Rpc<erpc::CTransport> *>(client->getRpc());

    auto *resp_msgbuf = respHandle->get_resp_msgbuf();
    auto resultMsg = reinterpret_cast<DaqdbDhtResult *>(resp_msgbuf->buf);
    reqCtx->rc = resultMsg->rc;

    reqCtx->ready = true;
    reqCtx->cv.notify_all();
    rpc->release_response(respHandle);
}

static void clbRemove(erpc::RespHandle *respHandle, void *ctxClient,
                      size_t ioCtx) {
    DhtClient *client = reinterpret_cast<DhtClient *>(ctxClient);
    DhtReqCtx *reqCtx = reinterpret_cast<DhtReqCtx *>(ioCtx);
    auto rpc =
        reinterpret_cast<erpc::Rpc<erpc::CTransport> *>(client->getRpc());

    auto *resp_msgbuf = respHandle->get_resp_msgbuf();
    auto resultMsg = reinterpret_cast<DaqdbDhtResult *>(resp_msgbuf->buf);
    reqCtx->rc = resultMsg->rc;

    reqCtx->ready = true;
    reqCtx->cv.notify_all();
    rpc->release_response(respHandle);
}

DhtClient::DhtClient(DhtCore *dhtCore, unsigned short port)
    : _dhtCore(dhtCore), _clientRpc(nullptr), _nexus(nullptr),
      state(DhtClientState::DHT_CLIENT_INIT) {}

DhtClient::~DhtClient() {
    if (_clientRpc)
        delete reinterpret_cast<erpc::Rpc<erpc::CTransport> *>(_clientRpc);
    if (_nexus)
        delete reinterpret_cast<erpc::Nexus *>(_nexus);
}

void DhtClient::_initializeNode(DhtNode *node) {
    auto serverUri = boost::str(boost::format("%1%:%2%") % node->getIp() %
                                to_string(node->getPort()));
    auto sessionNum =
        reinterpret_cast<erpc::Rpc<erpc::CTransport> *>(_clientRpc)
            ->create_session(serverUri, 0);

    node->setSessionId(sessionNum);
}

void DhtClient::initialize() {
    auto nexus =
        new erpc::Nexus(_dhtCore->getLocalNode()->getClientUri(), 0, 0);
    _nexus = nexus;
    auto rpc = new erpc::Rpc<erpc::CTransport>(nexus, this, 1, sm_handler);
    _clientRpc = rpc;

    try {
        auto neighbors = _dhtCore->getNeighbors();
        _initializeNode(_dhtCore->getLocalNode());
        for (DhtNode *neighbor : *neighbors) {
            _initializeNode(neighbor);
        }
        state = DhtClientState::DHT_CLIENT_READY;
        rpc->run_event_loop(DHT_CLIENT_POLL_INTERVAL);
    } catch (...) {
        state = DhtClientState::DHT_CLIENT_ERROR;
    }
}

Value DhtClient::get(const Key &key) {
    auto rpc = reinterpret_cast<erpc::Rpc<erpc::CTransport> *>(_clientRpc);
    DhtReqCtx reqCtx;

    auto reqSize = sizeof(DaqdbDhtMsg) + key.size();
    // auto req = rpc->alloc_msg_buffer_or_die(rpc->get_max_msg_size());
    auto req = rpc->alloc_msg_buffer_or_die(DEFAULT_ERPC_REQEST_BUF_SIZE);
    auto resp =
        rpc->alloc_msg_buffer_or_die(DEFAULT_ERPC_RESPONSE_PREALOC_SIZE);

    DaqdbDhtMsg *msg = reinterpret_cast<DaqdbDhtMsg *>(req.buf);
    msg->keySize = key.size();
    memcpy(msg->msg, key.data(), key.size());

    auto hostToSend = _dhtCore->getHostForKey(key);

    // @TODO jradtke passing context pointer using size_t
    assert(sizeof(size_t) == sizeof(&reqCtx));
    rpc->enqueue_request(
        hostToSend->getSessionId(),
        static_cast<unsigned char>(ErpRequestType::ERP_REQUEST_GET), &req,
        &resp, clbGet, reinterpret_cast<size_t>(&reqCtx));
    // wait for completion
    {
        unique_lock<mutex> lk(reqCtx.mtx);
        reqCtx.cv.wait_for(lk, 1s, [&reqCtx, &rpc] {
            rpc->run_event_loop(DHT_CLIENT_POLL_INTERVAL);
            return reqCtx.ready;
        });
    }
    if (reqCtx.rc != 0 || (reqCtx.value == nullptr)) {
        throw OperationFailedException(Status(KEY_NOT_FOUND));
    }

    return *reqCtx.value;
}

void DhtClient::put(const Key &key, const Value &val) {
    auto rpc = reinterpret_cast<erpc::Rpc<erpc::CTransport> *>(_clientRpc);
    DhtReqCtx reqCtx;

    auto reqSize = sizeof(DaqdbDhtMsg) + key.size() + val.size();
    auto req = rpc->alloc_msg_buffer_or_die(reqSize);
    DaqdbDhtMsg *msg = reinterpret_cast<DaqdbDhtMsg *>(req.buf);
    msg->keySize = key.size();
    memcpy(msg->msg, key.data(), key.size());
    msg->valSize = val.size();
    memcpy(msg->msg + key.size(), val.data(), msg->valSize);

    // TODO: jradtke Performance optimization possible if previous resp_msgbuf
    // reused
    auto resp = rpc->alloc_msg_buffer_or_die(sizeof(DaqdbDhtResult));
    auto hostToSend = _dhtCore->getHostForKey(key);

    // @TODO jradtke passing context pointer using size_t
    assert(sizeof(size_t) == sizeof(&reqCtx));
    rpc->enqueue_request(
        hostToSend->getSessionId(),
        static_cast<unsigned char>(ErpRequestType::ERP_REQUEST_PUT), &req,
        &resp, clbPut, reinterpret_cast<size_t>(&reqCtx));
    // wait for completion
    {
        unique_lock<mutex> lk(reqCtx.mtx);
        reqCtx.cv.wait_for(lk, 1s, [&reqCtx, &rpc] {
            rpc->run_event_loop(DHT_CLIENT_POLL_INTERVAL);
            return reqCtx.ready;
        });
    }
    if (reqCtx.rc != 0) {
        throw OperationFailedException(Status(UNKNOWN_ERROR));
    }
}

void DhtClient::remove(const Key &key) {
    auto rpc = reinterpret_cast<erpc::Rpc<erpc::CTransport> *>(_clientRpc);
    DhtReqCtx reqCtx;

    auto reqSize = sizeof(DaqdbDhtMsg) + key.size();
    auto req = rpc->alloc_msg_buffer_or_die(reqSize);
    DaqdbDhtMsg *msg = reinterpret_cast<DaqdbDhtMsg *>(req.buf);
    msg->keySize = key.size();
    memcpy(msg->msg, key.data(), key.size());

    auto bufferSize = 16 * 1024;
    auto resp = rpc->alloc_msg_buffer_or_die(bufferSize);

    auto hostToSend = _dhtCore->getHostForKey(key);

    // @TODO jradtke passing context pointer using size_t
    assert(sizeof(size_t) == sizeof(&reqCtx));
    rpc->enqueue_request(
        hostToSend->getSessionId(),
        static_cast<unsigned char>(ErpRequestType::ERP_REQUEST_REMOVE), &req,
        &resp, clbRemove, reinterpret_cast<size_t>(&reqCtx));
    // wait for completion
    {
        unique_lock<mutex> lk(reqCtx.mtx);
        reqCtx.cv.wait_for(lk, 1s, [&reqCtx, &rpc] {
            rpc->run_event_loop(DHT_CLIENT_POLL_INTERVAL);
            return reqCtx.ready;
        });
    }
    if (reqCtx.rc != 0) {
        throw OperationFailedException(Status(UNKNOWN_ERROR));
    }
}

bool DhtClient::ping(DhtNode &node) {
    auto rpc = reinterpret_cast<erpc::Rpc<erpc::CTransport> *>(_clientRpc);

    if (node.getSessionId() != ERPC_SESSION_NOT_SET) {
        return rpc->is_connected(node.getSessionId());
    } else {
        return false;
    }
}

} // namespace DaqDB
