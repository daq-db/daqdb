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

static erpc::MsgBuffer *erpcPrepareMsgbuf(erpc::Rpc<erpc::CTransport> *rpc,
                                          erpc::ReqHandle *req_handle,
                                          size_t reqSize) {
    erpc::MsgBuffer *msgBuf;

    if (reqSize <= rpc->get_max_data_per_pkt()) {
        msgBuf = &req_handle->pre_resp_msgbuf;
        rpc->resize_msg_buffer(msgBuf, reqSize);
    } else {
        req_handle->dyn_resp_msgbuf = rpc->alloc_msg_buffer_or_die(reqSize);
        msgBuf = &req_handle->dyn_resp_msgbuf;
    }

    return msgBuf;
}

static void erpcReqGetHandler(erpc::ReqHandle *req_handle, void *ctx) {
    DAQ_DEBUG("Get request received");
    auto serverCtx = reinterpret_cast<DhtServerCtx *>(ctx);
    auto rpc = reinterpret_cast<erpc::Rpc<erpc::CTransport> *>(serverCtx->rpc);

    auto req = req_handle->get_req_msgbuf();
    auto *msg = reinterpret_cast<DaqdbDhtMsg *>(req->buf);
    erpc::MsgBuffer *resp;

    try {
        // todo why cannot we set arbitrary response size?
        resp = erpcPrepareMsgbuf(rpc, req_handle, req->get_data_size());
        DaqdbDhtResult *result = reinterpret_cast<DaqdbDhtResult *>(resp->buf);
        result->msgSize = req->get_data_size();
        serverCtx->kvs->Get(msg->msg, msg->keySize, result->msg,
                            &result->msgSize);
        result->status = StatusCode::OK;
    } catch (DaqDB::OperationFailedException &e) {
        resp = erpcPrepareMsgbuf(rpc, req_handle, sizeof(DaqdbDhtResult));
        DaqdbDhtResult *result = reinterpret_cast<DaqdbDhtResult *>(resp->buf);
        result->msgSize = 0;
        result->status = e.status().getStatusCode();
    }

    rpc->enqueue_response(req_handle, resp);
    DAQ_DEBUG("Response enqueued");
}

static void erpcReqPutHandler(erpc::ReqHandle *req_handle, void *ctx) {
    DAQ_DEBUG("Put request received");
    auto serverCtx = reinterpret_cast<DhtServerCtx *>(ctx);
    auto rpc = reinterpret_cast<erpc::Rpc<erpc::CTransport> *>(serverCtx->rpc);

    auto req = req_handle->get_req_msgbuf();
    auto *msg = reinterpret_cast<DaqdbDhtMsg *>(req->buf);

    char *value;
    serverCtx->kvs->Alloc(msg->msg, msg->keySize, &value, msg->valSize);
    std::memcpy(value, msg->msg + msg->keySize, msg->valSize);
    erpc::MsgBuffer *resp =
        erpcPrepareMsgbuf(rpc, req_handle, sizeof(DaqdbDhtResult));
    DaqdbDhtResult *result = reinterpret_cast<DaqdbDhtResult *>(resp->buf);

    try {
        serverCtx->kvs->Put(msg->msg, msg->keySize, value, msg->valSize);
        result->msgSize = 0;
        result->status = StatusCode::OK;
    } catch (DaqDB::OperationFailedException &e) {
        result->msgSize = 0;
        result->status = e.status().getStatusCode();
    }

    rpc->enqueue_response(req_handle, resp);
    DAQ_DEBUG("Response enqueued");
}

static void erpcReqRemoveHandler(erpc::ReqHandle *req_handle, void *ctx) {
    DAQ_DEBUG("Remove request received");
    auto serverCtx = reinterpret_cast<DhtServerCtx *>(ctx);
    auto rpc = reinterpret_cast<erpc::Rpc<erpc::CTransport> *>(serverCtx->rpc);

    auto req = req_handle->get_req_msgbuf();
    auto *msg = reinterpret_cast<DaqdbDhtMsg *>(req->buf);

    erpc::MsgBuffer *resp =
        erpcPrepareMsgbuf(rpc, req_handle, sizeof(DaqdbDhtResult));
    DaqdbDhtResult *result = reinterpret_cast<DaqdbDhtResult *>(resp->buf);

    try {
        serverCtx->kvs->Remove(msg->msg, msg->keySize);
        result->msgSize = 0;
        result->status = StatusCode::OK;
    } catch (DaqDB::OperationFailedException &e) {
        result->msgSize = 0;
        result->status = e.status().getStatusCode();
    }

    rpc->enqueue_response(req_handle, resp);
    DAQ_DEBUG("Response enqueued");
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
