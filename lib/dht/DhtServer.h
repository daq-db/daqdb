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

#include <atomic>
#include <cstdint>
#include <string>
#include <thread>

#include <asio/io_service.hpp>

#include <EpollServer.h>

#include <DhtCore.h>
#include <DhtNode.h>
#include <Key.h>
#include <Value.h>

namespace DaqDB {

namespace zh = iit::datasys::zht::dm;

enum class DhtServerState : std::uint8_t {
    DHT_SERVER_INIT = 0,
    DHT_SERVER_READY,
    DHT_SERVER_ERROR,
    DHT_SERVER_STOPPED
};

class DhtServer {
  public:
    DhtServer(asio::io_service &io_service, DhtCore *dhtCore, KVStoreBase *kvs,
              unsigned short port);

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

    /**
     * Prints DHT status.
     * @return
     */
    std::string printStatus();
    /**
     * Prints DHT neighbors.
     * @return
     */
    std::string printNeighbors();

    inline ZHTClient *getClient() { return &_dhtCore->getClient()->c; }

    /**
     *
     */
    void serve(void);

    inline unsigned int getId() { return _dhtCore->getLocalNode()->getId(); }
    inline const std::string getIp() {
        return _dhtCore->getLocalNode()->getIp();
    }
    inline unsigned int getPort() {
        return _dhtCore->getLocalNode()->getPort();
    }
    inline unsigned int getHashMask() {
        return _dhtCore->getLocalNode()->getMask();
    }

    std::atomic<DhtServerState> state;

  private:
    void _serve(void);

    KVStoreBase *_kvs;

    DhtCore *_dhtCore;

    std::unique_ptr<zh::EpollServer> _spEpollServer;

    std::thread *_thread;
};

} // namespace DaqDB
