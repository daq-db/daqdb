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

#pragma once

#include <map>
#include <mutex>
#include <thread>

#include <condition_variable>

#include "DhtNode.h"
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
    int rc;
    Value *value;
    std::mutex mtx;
    std::condition_variable cv;
    bool ready = false;
};

class DhtCore;
class DhtClient {
  public:
    DhtClient(DhtCore *dhtCore, unsigned short port);

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

    bool ping(DhtNode &node);

    inline void *getRpc() { return _clientRpc; };

    std::atomic<bool> keepRunning;
    std::atomic<DhtClientState> state;

  private:
    void _initializeNode(DhtNode *node);

    // needed for class destructor only
    void *_clientRpc;
    void *_nexus;

    DhtCore *_dhtCore;
};
} // namespace DaqDB
