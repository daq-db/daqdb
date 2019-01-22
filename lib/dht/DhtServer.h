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

#pragma once

#include <atomic>
#include <cstdint>
#include <string>
#include <thread>

#include <DhtCore.h>
#include <DhtNode.h>
#include <KVStore.h>
#include <Key.h>
#include <Value.h>

namespace DaqDB {

enum class DhtServerState : std::uint8_t {
    DHT_SERVER_INIT = 0,
    DHT_SERVER_READY,
    DHT_SERVER_ERROR,
    DHT_SERVER_STOPPED
};

struct DhtServerCtx {
    void *rpc;
    KVStoreBase *kvs;
};

class DhtServer {
  public:
    DhtServer(DhtCore *dhtCore, KVStore *kvs, unsigned short port);
    ~DhtServer();

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

    void serve(void);

    inline unsigned int getId() {
        return _dhtCore->getLocalNode()->getSessionId();
    }
    inline const std::string getIp() {
        return _dhtCore->getLocalNode()->getIp();
    }
    inline unsigned int getPort() {
        return _dhtCore->getLocalNode()->getPort();
    }

    std::atomic<bool> keepRunning;
    std::atomic<DhtServerState> state;

  private:
    void _serve(void);

    KVStore *_kvs;
    DhtCore *_dhtCore;
    std::thread *_thread;
};

} // namespace DaqDB
