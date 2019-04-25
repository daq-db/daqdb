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

#pragma once

#include <rpc.h> /* include linux/if.h */

#include <map>
#include <mutex>
#include <thread>

#include <condition_variable>

#include "DhtNode.h"
#include "DhtTypes.h"
#include <KVStoreBase.h>
#include <Key.h>
#include <Value.h>

namespace DaqDB {

enum class DhtClientState : std::uint8_t {
    DHT_CLIENT_INIT = 0,
    DHT_CLIENT_READY,
    DHT_CLIENT_ERROR,
    DHT_CLIENT_STOPPED
};

struct DhtReqCtx {
    StatusCode status;
    union {
        Value *value;
        Key *key;
    };
    bool ready = false;
};

class DhtCore;
class DhtClient {
  public:
    DhtClient();
    DhtClient(uint8_t remotRpcIdBase);
    virtual ~DhtClient();

    /**
     * Initializes internal buffers and setup eRPC environment (Rpc endpoint,
     * sessions, etc.)
     */
    void initialize(DhtCore *dhtCore);

    /**
     * Synchronously get a value for a given key.
     * Remote node is calculated based on key hash (see DhtCore._genHash).
     *
     * @param key Reference to a key structure
     *
     * @throw OperationFailedException if any error occurred
     *
     * @return On success returns allocated buffer with value. The caller is
     * responsible of releasing the buffer.
     */
    Value get(const Key &key);

    /**
     * Synchronously get any key.
     * Remote node is calculated based on round robin.
     *
     * @param key Reference to a key structure
     *
     * @throw OperationFailedException if any error occurred
     *
     * @return On success returns allocated buffer with key. The caller is
     * responsible of releasing the buffer.
     */
    Key getAny();

    /**
     * Synchronously insert a value for a given key.
     * Remote node is calculated based on key hash (see DhtCore._genHash).
     *
     * @param key Reference to a key structure
     * @param val Reference to a value structure
     *
     * @throw OperationFailedException if any error occurred
     */
    void put(const Key &key, const Value &val);

    /**
     * Synchronously remove a key-value store entry for a given key.
     * Remote node is calculated based on key hash (see DhtCore._genHash).
     *
     * @throw OperationFailedException if any error occurred
     *
     * @param key Reference to a key structure
     */
    void remove(const Key &key);

    Key allocKey(size_t keySize);
    void free(Key &&key);
    Value alloc(const Key &key, size_t size);
    void free(const Key &key, Value &&value);

    erpc::MsgBuffer *getRespMsgBuf();

    bool ping(DhtNode &node);

    /**
     * @return erpc::Rpc<erpc::CTransport> object
     */
    inline erpc::Rpc<erpc::CTransport> *getRpc() { return _clientRpc; };

    /**
     * @param newRpc pointer to erpc::Rpc<erpc::CTransport> object
     */
    void setRpc(erpc::Rpc<erpc::CTransport> *newRpc);

    inline DhtReqCtx *getReqCtx() { return &_reqCtx; };

    void setReqCtx(DhtReqCtx &reqCtx);

    /**
     * Resize MsgBuffers to fit a request and response.
     *
     * @param new_request_size The new size in bytes of the _reqMsgBuf. This
     * must be smaller than the size used to create the MsgBuffer in
     * alloc_msg_buffer().
     * @param new_response_size The new size in bytes of the _respMsgBuf. This
     * must be smaller than the size used to create the MsgBuffer in
     * alloc_msg_buffer().
     * @note public and virtual to allow mocking in unit tests
     */
    virtual void resizeMsgBuffers(size_t new_request_size,
                                  size_t new_response_size);

    /**
     * Enqueue request of given type to Rpc and waits for completion.
     *
     * @param target Host DHT node where request is send to
     * @param type operation type
     * @param contFunc callback function
     * @note public and virtual to allow mocking in unit tests
     */
    virtual void enqueueAndWait(DhtNode *targetHost, ErpRequestType type,
                                DhtContFunc contFunc);

    /**
     * Fills request buffer.
     *
     * @param key Pointer to a key structure, optional
     * @param val Pointer to a value structure, optional
     * @note public and virtual to allow mocking in unit tests
     */
    virtual void fillReqMsg(const Key *key, const Value *val);

    /**
     * Finds DHT node that stores given key.
     *
     * @param key Reference to a key structure
     * @return DHT node that stores the key
     * @note public and virtual to allow mocking in unit tests
     */
    virtual DhtNode *getTargetHost(const Key &key);

    /**
     * Returns any DHT node.
     *
     * @return DHT node
     * @note public and virtual to allow mocking in unit tests
     */
    virtual DhtNode *getAnyHost(void);

    std::atomic<DhtClientState> state;

  private:
    void _initializeNode(DhtNode *node);
    void _runToResponse();
    void _initReqCtx();

    erpc::Rpc<erpc::CTransport> *_clientRpc;
    erpc::Nexus *_nexus;

    DhtCore *_dhtCore;

    struct DhtReqCtx _reqCtx;
    std::unique_ptr<erpc::MsgBuffer> _reqMsgBuf;
    std::unique_ptr<erpc::MsgBuffer> _respMsgBuf;
    bool _reqMsgBufInUse = false;
    bool _reqMsgBufValInUse = false;
    uint8_t _remoteRpcId = 0;
};
} // namespace DaqDB
