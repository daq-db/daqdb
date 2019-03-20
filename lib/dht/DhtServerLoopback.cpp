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
#include "DhtTypes.h"
#include <Status.h>

#include "DhtServerLoopback.h"

namespace DaqDB {

void erpcLoopbackReqGetHandler(erpc::ReqHandle *req_handle, void *ctx) {
    auto serverCtx = reinterpret_cast<DhtServerCtx *>(ctx);
    auto rpc = reinterpret_cast<erpc::Rpc<erpc::CTransport> *>(serverCtx->rpc);

    erpc::MsgBuffer *resp = &req_handle->pre_resp_msgbuf;
    rpc->resize_msg_buffer(resp, sizeof(DaqdbDhtResult));

    DaqdbDhtResult *result = reinterpret_cast<DaqdbDhtResult *>(resp->buf);
    result->msgSize = 0;
    result->status = StatusCode::OK;

    rpc->enqueue_response(req_handle, resp);
}

void erpcLoopbackReqPutHandler(erpc::ReqHandle *req_handle, void *ctx) {
    auto serverCtx = reinterpret_cast<DhtServerCtx *>(ctx);
    auto rpc = reinterpret_cast<erpc::Rpc<erpc::CTransport> *>(serverCtx->rpc);

    erpc::MsgBuffer *resp = &req_handle->pre_resp_msgbuf;
    rpc->resize_msg_buffer(resp, sizeof(DaqdbDhtResult));

    DaqdbDhtResult *result = reinterpret_cast<DaqdbDhtResult *>(resp->buf);
    result->msgSize = 0;
    result->status = StatusCode::OK;

    rpc->enqueue_response(req_handle, resp);
}

void erpcLoopbackReqRemoveHandler(erpc::ReqHandle *req_handle, void *ctx) {
    auto serverCtx = reinterpret_cast<DhtServerCtx *>(ctx);
    auto rpc = reinterpret_cast<erpc::Rpc<erpc::CTransport> *>(serverCtx->rpc);

    erpc::MsgBuffer *resp = &req_handle->pre_resp_msgbuf;
    rpc->resize_msg_buffer(resp, sizeof(DaqdbDhtResult));

    DaqdbDhtResult *result = reinterpret_cast<DaqdbDhtResult *>(resp->buf);
    result->msgSize = 0;
    result->status = StatusCode::OK;

    rpc->enqueue_response(req_handle, resp);
}

void erpcLoopbackReqGetAnyHandler(erpc::ReqHandle *req_handle, void *ctx) {
    auto serverCtx = reinterpret_cast<DhtServerCtx *>(ctx);
    auto rpc = reinterpret_cast<erpc::Rpc<erpc::CTransport> *>(serverCtx->rpc);

    erpc::MsgBuffer *resp = &req_handle->pre_resp_msgbuf;
    rpc->resize_msg_buffer(resp, sizeof(DaqdbDhtResult));

    DaqdbDhtResult *result = reinterpret_cast<DaqdbDhtResult *>(resp->buf);
    result->msgSize = 0;
    result->status = StatusCode::OK;

    rpc->enqueue_response(req_handle, resp);
}

} // namespace DaqDB
