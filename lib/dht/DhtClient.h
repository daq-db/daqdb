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

#pragma once

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
    Value *value;
    bool ready = false;
};

class DhtCore;
class DhtClient {
  public:
    DhtClient();
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
    inline void *getRpc() { return _clientRpc; };

    /**
     * @param newRpc pointer to erpc::Rpc<erpc::CTransport> object
     */
    void setRpc(void *newRpc);

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

    std::atomic<DhtClientState> state;

  private:
    void _initializeNode(DhtNode *node);
    void _runToResponse();
    void _initReqCtx();

    void *_clientRpc; // Stores erpc::Rpc<erpc::CTransport>
    erpc::Nexus *_nexus;

    DhtCore *_dhtCore;

    struct DhtReqCtx _reqCtx;
    std::unique_ptr<erpc::MsgBuffer> _reqMsgBuf;
    std::unique_ptr<erpc::MsgBuffer> _respMsgBuf;
    bool _reqMsgBufInUse = false;
    bool _reqMsgBufValInUse = false;
};
} // namespace DaqDB
