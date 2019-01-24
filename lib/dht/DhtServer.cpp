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

#include "DhtServer.h"

#include <iostream>
#include <sstream>

#include <boost/assign/list_of.hpp>
#include <boost/format.hpp>

#include <Logger.h>
#include <rpc.h>

namespace DaqDB {

using namespace std;
using boost::format;

map<DhtNodeState, string> NodeStateStr =
    boost::assign::map_list_of(DhtNodeState::NODE_READY, "Ready")(
        DhtNodeState::NODE_NOT_RESPONDING,
        "Not Responding")(DhtNodeState::NODE_INIT, "Not initialized");

static void erpcReqGetHandler(erpc::ReqHandle *req_handle, void *ctx) {
    auto serverCtx = reinterpret_cast<DhtServerCtx *>(ctx);
    auto rpc = reinterpret_cast<erpc::Rpc<erpc::CTransport> *>(serverCtx->rpc);

    auto req = req_handle->get_req_msgbuf();
    auto *msg = reinterpret_cast<DaqdbDhtMsg *>(req->buf);

    /*
     * Allocation from DHT buffer not needed for local request, for
     * remote one DHT client method will handle buffering.
     */
    // @TODO jradtke AllocKey need changes to avoid extra memory copying in
    // kvs->Get
    Key key = serverCtx->kvs->AllocKey(KeyValAttribute::NOT_BUFFERED);
    memcpy(key.data(), msg->msg, msg->keySize);

    try {
        auto val = serverCtx->kvs->Get(key);

        auto reqMsgbufSize = req->get_data_size();
        auto responseSize = val.size() + sizeof(DaqdbDhtResult);

        req_handle->prealloc_used = false;
        req_handle->dyn_resp_msgbuf =
            rpc->alloc_msg_buffer_or_die(reqMsgbufSize);
        erpc::MsgBuffer &resp_msgbuf = req_handle->dyn_resp_msgbuf;

        DaqdbDhtResult *result =
            reinterpret_cast<DaqdbDhtResult *>(resp_msgbuf.buf);

        result->msgSize = val.size();
        if (result->msgSize > 0) {
            memcpy(result->msg, val.data(), result->msgSize);
        }
        result->status = StatusCode::OK;
    } catch (DaqDB::OperationFailedException &e) {
        auto &resp = req_handle->pre_resp_msgbuf;
        rpc->resize_msg_buffer(&resp, sizeof(DaqdbDhtResult));
        DaqdbDhtResult *result = reinterpret_cast<DaqdbDhtResult *>(resp.buf);
        result->msgSize = 0;
        req_handle->prealloc_used = true;

        result->status = e.status().getStatusCode();
    }

    rpc->enqueue_response(req_handle);
}

static void erpcReqPutHandler(erpc::ReqHandle *req_handle, void *ctx) {
    auto serverCtx = reinterpret_cast<DhtServerCtx *>(ctx);
    auto rpc = reinterpret_cast<erpc::Rpc<erpc::CTransport> *>(serverCtx->rpc);

    auto req = req_handle->get_req_msgbuf();
    auto *msg = reinterpret_cast<DaqdbDhtMsg *>(req->buf);

    Key key = serverCtx->kvs->AllocKey(KeyValAttribute::NOT_BUFFERED);
    std::memcpy(key.data(), msg->msg, msg->keySize);
    Value value = serverCtx->kvs->Alloc(key, msg->valSize);
    std::memcpy(value.data(), msg->msg + msg->keySize, msg->valSize);

    auto &resp = req_handle->pre_resp_msgbuf;
    req_handle->prealloc_used = true;
    try {
        serverCtx->kvs->Put(move(key), move(value));
        rpc->resize_msg_buffer(&resp, sizeof(DaqdbDhtResult));
        DaqdbDhtResult *result = reinterpret_cast<DaqdbDhtResult *>(resp.buf);
        result->msgSize = 0;
        result->status = StatusCode::OK;
    } catch (DaqDB::OperationFailedException &e) {
        rpc->resize_msg_buffer(&resp, sizeof(DaqdbDhtResult));
        DaqdbDhtResult *result = reinterpret_cast<DaqdbDhtResult *>(resp.buf);
        result->msgSize = 0;
        result->status = e.status().getStatusCode();
    }

    rpc->enqueue_response(req_handle);
}

static void erpcReqRemoveHandler(erpc::ReqHandle *req_handle, void *ctx) {
    auto serverCtx = reinterpret_cast<DhtServerCtx *>(ctx);
    auto rpc = reinterpret_cast<erpc::Rpc<erpc::CTransport> *>(serverCtx->rpc);

    auto req = req_handle->get_req_msgbuf();
    auto *msg = reinterpret_cast<DaqdbDhtMsg *>(req->buf);

    /*
     * Allocation from DHT buffer not needed for local request, for
     * remote one DHT client method will handle buffering.
     */
    // @TODO jradtke AllocKey need changes to avoid extra memory copying in
    // kvs->Remove
    Key key = serverCtx->kvs->AllocKey(KeyValAttribute::NOT_BUFFERED);
    std::memcpy(key.data(), msg->msg, msg->keySize);

    auto &resp = req_handle->pre_resp_msgbuf;
    req_handle->prealloc_used = true;
    try {
        serverCtx->kvs->Remove(key);
        rpc->resize_msg_buffer(&resp, sizeof(DaqdbDhtResult));
        DaqdbDhtResult *result = reinterpret_cast<DaqdbDhtResult *>(resp.buf);
        result->msgSize = 0;
        result->status = StatusCode::OK;
    } catch (DaqDB::OperationFailedException &e) {
        rpc->resize_msg_buffer(&resp, sizeof(DaqdbDhtResult));
        DaqdbDhtResult *result = reinterpret_cast<DaqdbDhtResult *>(resp.buf);
        result->msgSize = 0;
        result->status = e.status().getStatusCode();
    }

    rpc->enqueue_response(req_handle);
}

DhtServer::DhtServer(DhtCore *dhtCore, KVStore *kvs)
    : state(DhtServerState::DHT_SERVER_INIT), _dhtCore(dhtCore), _kvs(kvs),
      _thread(nullptr) {
    serve();
}

DhtServer::~DhtServer() {
    keepRunning = false;
    if (state == DhtServerState::DHT_SERVER_READY && _thread != nullptr)
        _thread->join();
}

void DhtServer::_serve(void) {

    auto nexus = new erpc::Nexus(_dhtCore->getLocalNode()->getUri(), 0, 0);

    nexus->register_req_func(
        static_cast<unsigned int>(ErpRequestType::ERP_REQUEST_GET),
        erpcReqGetHandler);
    nexus->register_req_func(
        static_cast<unsigned int>(ErpRequestType::ERP_REQUEST_PUT),
        erpcReqPutHandler);
    nexus->register_req_func(
        static_cast<unsigned int>(ErpRequestType::ERP_REQUEST_REMOVE),
        erpcReqRemoveHandler);

    DhtServerCtx rpcCtx;
    rpcCtx.kvs = _kvs;

    erpc::Rpc<erpc::CTransport> *rpc;
    try {
        rpc = new erpc::Rpc<erpc::CTransport>(nexus, &rpcCtx, 0, nullptr);
        rpcCtx.rpc = rpc;
        state = DhtServerState::DHT_SERVER_READY;
        keepRunning = true;
        while (keepRunning) {
            rpc->run_event_loop_once();
        }
        state = DhtServerState::DHT_SERVER_STOPPED;

        if (rpc) {
            delete rpc;
        }
    } catch (exception &e) {
        DAQ_DEBUG("DHT server exception: " + std::string(e.what()));
        state = DhtServerState::DHT_SERVER_ERROR;
        throw;
    } catch (...) {
        DAQ_DEBUG("DHT server exception: unknown");
        state = DhtServerState::DHT_SERVER_ERROR;
        throw;
    }

    delete nexus;
}

void DhtServer::serve(void) {
    DAQ_DEBUG("Creating server thread");
    _thread = new thread(&DhtServer::_serve, this);
    do {
        sleep(1);
    } while (state == DhtServerState::DHT_SERVER_INIT);
    DAQ_DEBUG("Server thread running");
}

string DhtServer::printStatus() {
    stringstream result;

    result << "DHT server: " << getIp() << ":" << getPort() << endl;
    if (state == DhtServerState::DHT_SERVER_READY) {
        result << "DHT server: active" << endl;
    } else {
        result << "DHT server: inactive" << endl;
    }

    return result.str();
}

string DhtServer::printNeighbors() {

    stringstream result;

    auto neighbors = _dhtCore->getNeighbors();
    if (neighbors->size()) {
        for (auto neighbor : *neighbors) {

            auto isConnected = _dhtCore->getClient()->ping(*neighbor);
            if (isConnected) {
                neighbor->state = DhtNodeState::NODE_READY;
            } else {
                neighbor->state = DhtNodeState::NODE_NOT_RESPONDING;
            }
            result << boost::str(
                boost::format("[%1%:%2%] - SessionId(%3%) : %4%\n") %
                neighbor->getIp() % to_string(neighbor->getPort()) %
                neighbor->getSessionId() % NodeStateStr[neighbor->state]);
        }
    } else {
        result << "No neighbors";
    }

    return result.str();
}

} // namespace DaqDB
