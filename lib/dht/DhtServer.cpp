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

#include "DhtServer.h"

#include <iostream>
#include <sstream>

#include <boost/assign/list_of.hpp>
#include <boost/format.hpp>

#include <Logger.h>
#include <rpc.h>

/** @TODO jradtke: should be taken from configuration file */
#define DHT_SERVER_CPU_CORE_BASE 5
#define DHT_SERVER_CPU_CORE_MAX 32

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

static void erpcReqGetAnyHandler(erpc::ReqHandle *req_handle, void *ctx) {
    DAQ_DEBUG("GetAny request received");
    auto serverCtx = reinterpret_cast<DhtServerCtx *>(ctx);
    auto rpc = reinterpret_cast<erpc::Rpc<erpc::CTransport> *>(serverCtx->rpc);

    auto req = req_handle->get_req_msgbuf();
    auto *msg = reinterpret_cast<DaqdbDhtMsg *>(req->buf);
    erpc::MsgBuffer *resp;

    try {
        resp =
            erpcPrepareMsgbuf(rpc, req_handle, sizeof(DaqdbDhtResult) +
                                                   serverCtx->kvs->KeySize());
        DaqdbDhtResult *result = reinterpret_cast<DaqdbDhtResult *>(resp->buf);
        result->msgSize = serverCtx->kvs->KeySize();
        serverCtx->kvs->GetAny(result->msg, serverCtx->kvs->KeySize());
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

static void erpcReqGetHandler(erpc::ReqHandle *req_handle, void *ctx) {
    DAQ_DEBUG("Get request received");
    auto serverCtx = reinterpret_cast<DhtServerCtx *>(ctx);
    auto rpc = reinterpret_cast<erpc::Rpc<erpc::CTransport> *>(serverCtx->rpc);

    auto req = req_handle->get_req_msgbuf();
    auto *msg = reinterpret_cast<DaqdbDhtMsg *>(req->buf);
    erpc::MsgBuffer *resp;

    try {
        resp = erpcPrepareMsgbuf(rpc, req_handle, ERPC_MAX_RESPONSE_SIZE);
        DaqdbDhtResult *result = reinterpret_cast<DaqdbDhtResult *>(resp->buf);
        result->msgSize = ERPC_MAX_RESPONSE_SIZE - sizeof(DaqdbDhtResult);
        serverCtx->kvs->Get(msg->msg, msg->keySize, result->msg,
                            &result->msgSize);
        rpc->resize_msg_buffer(resp, sizeof(DaqdbDhtResult) + result->msgSize);
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

DhtServer::DhtServer(DhtCore *dhtCore, KVStore *kvs, uint8_t numWorkerThreads)
    : state(DhtServerState::DHT_SERVER_INIT), _dhtCore(dhtCore), _kvs(kvs),
      _thread(nullptr), _workerThreadsNumber(numWorkerThreads) {
    serve();
}

DhtServer::~DhtServer() {
    keepRunning = false;
    if (state == DhtServerState::DHT_SERVER_READY && _thread != nullptr)
        _thread->join();
}

void DhtServer::_serveWorker(unsigned int workerId, cpu_set_t *cpuset, size_t size) {
    DhtServerCtx rpcCtx;

    const int set_result = pthread_setaffinity_np(pthread_self(),
                                                  size, cpuset);
    if (!set_result) {
        DAQ_DEBUG("Cannot set affinity for DHT server worker[" +
                  to_string(workerId) + "]");
    }

    try {
        erpc::Rpc<erpc::CTransport> *rpc = new erpc::Rpc<erpc::CTransport>(
            _spServerNexus.get(), &rpcCtx, workerId, nullptr);
        rpcCtx.kvs = _kvs;
        rpcCtx.rpc = rpc;
        while (keepRunning) {
            rpc->run_event_loop_once();
        }
        if (rpc) {
            delete rpc;
        }
    } catch (exception &e) {
        DAQ_DEBUG("DHT server worker [" + to_string(workerId) +
                  "] exception: " + std::string(e.what()));
    } catch (...) {
        DAQ_DEBUG("DHT server worker [" + to_string(workerId) +
                  "] exception: unknown");
    }
}

void DhtServer::_serve(void) {

    size_t size = CPU_ALLOC_SIZE(DHT_SERVER_CPU_CORE_MAX);
    cpu_set_t *cpuset = CPU_ALLOC(DHT_SERVER_CPU_CORE_MAX);
    if (!cpuset) {
        state = DhtServerState::DHT_SERVER_ERROR;
        DAQ_DEBUG("Cannot allocate cpuset");
        throw OperationFailedException(ALLOCATION_ERROR);
    }

    CPU_ZERO_S(size, cpuset);
    CPU_SET_S(DHT_SERVER_CPU_CORE_BASE, size, cpuset);

    const int set_result = pthread_setaffinity_np(pthread_self(),
                                                  size, cpuset);
    if (!set_result) {
        DAQ_DEBUG("Cannot set affinity for DHT server thread");
    }

    _spServerNexus.reset(
        new erpc::Nexus(_dhtCore->getLocalNode()->getUri(), 0, 0));

    _spServerNexus->register_req_func(
        static_cast<unsigned int>(ErpRequestType::ERP_REQUEST_GET),
        erpcReqGetHandler);
    _spServerNexus->register_req_func(
        static_cast<unsigned int>(ErpRequestType::ERP_REQUEST_GETANY),
        erpcReqGetAnyHandler);
    _spServerNexus->register_req_func(
        static_cast<unsigned int>(ErpRequestType::ERP_REQUEST_PUT),
        erpcReqPutHandler);
    _spServerNexus->register_req_func(
        static_cast<unsigned int>(ErpRequestType::ERP_REQUEST_REMOVE),
        erpcReqRemoveHandler);

    try {
        DhtServerCtx rpcCtx;
        erpc::Rpc<erpc::CTransport> *rpc = new erpc::Rpc<erpc::CTransport>(
            _spServerNexus.get(), &rpcCtx, 0, nullptr);
        rpcCtx.kvs = _kvs;
        rpcCtx.rpc = rpc;

        for (uint8_t threadIndex = 1; threadIndex <= _workerThreadsNumber;
             ++threadIndex) {
            CPU_ZERO_S(size, cpuset);
            CPU_SET_S(DHT_SERVER_CPU_CORE_BASE + threadIndex, size, cpuset);
            _workerThreads.push_back(new thread(&DhtServer::_serveWorker, this,
                                                threadIndex, cpuset, size));
        }

        state = DhtServerState::DHT_SERVER_READY;
        keepRunning = true;
        while (keepRunning) {
            rpc->run_event_loop_once();
        }
        state = DhtServerState::DHT_SERVER_STOPPED;

        while (!_workerThreads.empty()) {
            auto workerThread = _workerThreads.back();
            _workerThreads.pop_back();
            workerThread->join();
            delete workerThread;
        }

        if (rpc) {
            delete rpc;
        }
    } catch (exception &e) {
        DAQ_DEBUG("DHT server exception: " + std::string(e.what()));
        state = DhtServerState::DHT_SERVER_ERROR;
        CPU_FREE(cpuset);
        throw;
    } catch (...) {
        DAQ_DEBUG("DHT server exception: unknown");
        state = DhtServerState::DHT_SERVER_ERROR;
        CPU_FREE(cpuset);
        throw;
    }

    CPU_FREE(cpuset);
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
