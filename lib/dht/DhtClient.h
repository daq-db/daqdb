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
#include <KVStoreBase.h>
#include <Key.h>
#include <Value.h>

// @TODO : jradtke Cannot include rpc.h, net / if.h conflict
namespace erpc {
class MsgBuffer;
}

namespace DaqDB {

enum class DhtClientState : std::uint8_t {
    DHT_CLIENT_INIT = 0,
    DHT_CLIENT_READY,
    DHT_CLIENT_ERROR,
    DHT_CLIENT_STOPPED
};

struct DhtReqCtx {
    int rc;
    Value *value;
    bool ready = false;
};

class DhtCore;
class DhtClient {
  public:
    DhtClient(DhtCore *dhtCore, unsigned short port);
    ~DhtClient();

    /**
     *
     */
    void initialize();

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
    void free(Value &&value); 

    bool ping(DhtNode &node);

    inline void *getRpc() { return _clientRpc; };
    inline DhtReqCtx *getReqCtx() { return &_reqCtx; };

    std::atomic<bool> keepRunning;
    std::atomic<DhtClientState> state;

  private:
    void _initializeNode(DhtNode *node);
    void _runToResponse();
    void _initReqCtx();

    // needed for class destructor only
    void *_clientRpc;
    void *_nexus;

    DhtCore *_dhtCore;

    struct DhtReqCtx  _reqCtx;
    std::unique_ptr<erpc::MsgBuffer> _reqMsgBuf;
    std::unique_ptr<erpc::MsgBuffer> _respMsgBuf;
    bool _reqMsgBufInUse = false;
};
} // namespace DaqDB
