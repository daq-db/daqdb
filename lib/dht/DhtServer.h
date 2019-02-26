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

#include <atomic>
#include <cstdint>
#include <string>
#include <thread>

#include <KVStore.h> /* net/if.h (put before linux/if.h) */

#include <DhtCore.h>
#include <DhtNode.h>
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
    KVStore *kvs;
};

class DhtServer {
  public:
    DhtServer(DhtCore *dhtCore, KVStore *kvs, uint8_t numWorkerThreads = 0);
    ~DhtServer();

    /**
     * Prints DHT status.
     * @return DHT status
     * e.g. "DHT server: localhost:31850
     *       DHT server: active"
     *
     */
    std::string printStatus();
    /**
     * Prints DHT neighbors.
     * @return string with list of peer storage nodes
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
    void _serveWorker(unsigned int workerId, cpu_set_t *cpuset, size_t size);

    std::unique_ptr<erpc::Nexus> _spServerNexus;
    KVStore *_kvs;
    DhtCore *_dhtCore;
    std::thread *_thread;

    uint8_t _workerThreadsNumber;
    std::vector<std::thread> _workerThreads;
};

} // namespace DaqDB
