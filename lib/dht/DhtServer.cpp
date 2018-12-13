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

#include "DhtServer.h"

#include <sstream>

#include <boost/assign/list_of.hpp>
#include <boost/format.hpp>

#include <rpc.h>

namespace DaqDB {

using namespace std;
using boost::format;

map<DhtNodeState, string> NodeStateStr =
    boost::assign::map_list_of(DhtNodeState::NODE_READY, "Ready")(
        DhtNodeState::NODE_NOT_RESPONDING,
        "Not Responding")(DhtNodeState::NODE_INIT, "Not initialized");

const size_t DEFAULT_ERPC_REQ_SIZE = 16;
const size_t DEFAULT_ERPC_RESPONSE_SIZE = 16;

static void erpcReqGetHandler(erpc::ReqHandle *req_handle, void *ctx) {
    auto serverCtx = reinterpret_cast<DhtServerCtx *>(ctx);
    auto rpc = reinterpret_cast<erpc::Rpc<erpc::CTransport> *>(serverCtx->rpc);

    auto req = req_handle->get_req_msgbuf();
    auto *msg = reinterpret_cast<DaqdbDhtMsg *>(req->buf);

    Key key = serverCtx->kvs->AllocKey();
    std::memcpy(key.data(), msg->msg, msg->keySize);

    auto &resp = req_handle->pre_resp_msgbuf;
    req_handle->prealloc_used = true;
    try {
        auto val = serverCtx->kvs->Get(key);
        auto responseSize = val.size() + sizeof(DaqdbDhtResult);
        rpc->resize_msg_buffer(&resp, responseSize);
        DaqdbDhtResult *result = reinterpret_cast<DaqdbDhtResult *>(resp.buf);
        result->rc = 0;
        result->msgSize = val.size();
        if (result->msgSize > 0) {
            memcpy(result->msg, val.data(), result->msgSize);
        }
    } catch (DaqDB::OperationFailedException &e) {
        rpc->resize_msg_buffer(&resp, sizeof(DaqdbDhtResult));
        DaqdbDhtResult *result = reinterpret_cast<DaqdbDhtResult *>(resp.buf);
        result->rc = 1;
        result->msgSize = 0;
    }

    rpc->enqueue_response(req_handle);
}

static void erpcReqPutHandler(erpc::ReqHandle *req_handle, void *ctx) {
    auto serverCtx = reinterpret_cast<DhtServerCtx *>(ctx);
    auto rpc = reinterpret_cast<erpc::Rpc<erpc::CTransport> *>(serverCtx->rpc);

    auto req = req_handle->get_req_msgbuf();
    auto *msg = reinterpret_cast<DaqdbDhtMsg *>(req->buf);

    Key key = serverCtx->kvs->AllocKey();
    std::memcpy(key.data(), msg->msg, msg->keySize);
    Value value = serverCtx->kvs->Alloc(key, msg->valSize);
    std::memcpy(value.data(), msg->msg + msg->keySize, msg->valSize);

    auto &resp = req_handle->pre_resp_msgbuf;
    req_handle->prealloc_used = true;
    try {

        serverCtx->kvs->Put(move(key), move(value));
        rpc->resize_msg_buffer(&resp, sizeof(DaqdbDhtResult));
        DaqdbDhtResult *result = reinterpret_cast<DaqdbDhtResult *>(resp.buf);
        result->rc = 0;
        result->msgSize = 0;
    } catch (DaqDB::OperationFailedException &e) {
        rpc->resize_msg_buffer(&resp, sizeof(DaqdbDhtResult));
        DaqdbDhtResult *result = reinterpret_cast<DaqdbDhtResult *>(resp.buf);
        result->rc = 1;
        result->msgSize = 0;
    }

    rpc->enqueue_response(req_handle);
}

static void erpcReqRemoveHandler(erpc::ReqHandle *req_handle, void *ctx) {
    auto serverCtx = reinterpret_cast<DhtServerCtx *>(ctx);
    auto rpc = reinterpret_cast<erpc::Rpc<erpc::CTransport> *>(serverCtx->rpc);

    auto req = req_handle->get_req_msgbuf();
    auto *msg = reinterpret_cast<DaqdbDhtMsg *>(req->buf);

    Key key = serverCtx->kvs->AllocKey();
    std::memcpy(key.data(), msg->msg, msg->keySize);

    auto &resp = req_handle->pre_resp_msgbuf;
    req_handle->prealloc_used = true;
    try {
        serverCtx->kvs->Remove(key);
        rpc->resize_msg_buffer(&resp, sizeof(DaqdbDhtResult));
        DaqdbDhtResult *result = reinterpret_cast<DaqdbDhtResult *>(resp.buf);
        result->rc = 0;
        result->msgSize = 0;
    } catch (DaqDB::OperationFailedException &e) {
        rpc->resize_msg_buffer(&resp, sizeof(DaqdbDhtResult));
        DaqdbDhtResult *result = reinterpret_cast<DaqdbDhtResult *>(resp.buf);
        result->rc = 1;
        result->msgSize = 0;
    }

    rpc->enqueue_response(req_handle);
}

DhtServer::DhtServer(DhtCore *dhtCore, KVStoreBase *kvs, unsigned short port)
    : state(DhtServerState::DHT_SERVER_INIT), _dhtCore(dhtCore), _kvs(kvs),
      _thread(nullptr) {
    serve();
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
    auto rpc = new erpc::Rpc<erpc::CTransport>(nexus, &rpcCtx, 0, nullptr);
    rpcCtx.rpc = rpc;

    try {
        state = DhtServerState::DHT_SERVER_READY;
        keepRunning = true;
        while (keepRunning) {
            rpc->run_event_loop(100);
        }
        state = DhtServerState::DHT_SERVER_STOPPED;
    } catch (...) {
        state = DhtServerState::DHT_SERVER_ERROR;
    }
    delete rpc;
}

void DhtServer::serve(void) {
    _thread = new thread(&DhtServer::_serve, this);
    do {
        sleep(1);
    } while (state == DhtServerState::DHT_SERVER_INIT);
}

string DhtServer::printStatus() {
    stringstream result;

    if (state == DhtServerState::DHT_SERVER_READY) {
        result << "DHT server: active";
    } else {
        result << "DHT server: inactive";
    }

    if (_dhtCore->getClient()->state == DhtClientState::DHT_CLIENT_READY) {
        result << "DHT client: active";
    } else {
        result << "DHT client: inactive";
    }

    return result.str();
}

string DhtServer::printNeighbors() {

    stringstream result;

    auto neighbors = _dhtCore->getNeighbors();
    if (neighbors->size()) {
        for (auto neighbor : *neighbors) {
            /*
            if (getClient()->ping(neighbor->getId()) ==
                zh::Const::toInt(zh::Const::ZSC_REC_SUCC)) {
                neighbor->state = DhtNodeState::NODE_READY;
            } else {
                neighbor->state = DhtNodeState::NODE_NOT_RESPONDING;
            }
            */
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
