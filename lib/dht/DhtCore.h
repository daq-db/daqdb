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

#include <map>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include <DhtClient.h> /* include linux/if.h */
#include <DhtNode.h>

#include "DhtTypes.h"
#include <daqdb/Key.h>
#include <daqdb/Options.h>

#define ERPC_DAQDB_METADATA_SIZE 64
#define ERPC_MAX_REQUEST_SIZE ((16 * 1024) + ERPC_DAQDB_METADATA_SIZE)
#define ERPC_MAX_RESPONSE_SIZE ((16 * 1024) + ERPC_DAQDB_METADATA_SIZE)

namespace DaqDB {
class DhtCore {
  public:
    /**
     * @note needed to allow mocking in unit tests
     */
    DhtCore();
    DhtCore(const DaqDB::DhtCore &);

    /**
     * @param dhtOptions dht options
     *
     * @note At the moment only satellite node should initialize nexus
     */
    DhtCore(DhtOptions dhtOptions);
    virtual ~DhtCore();

    /**
     * Creates and initializes DhtClient for caller thread.
     * @note public and virtual to allow mocking in unit tests
     */
    virtual void initClient();

    bool isLocalKey(Key key);

    DhtNode *getHostForKey(Key key);
    DhtNode *getHostAny();

    inline RangeToHost *getRangeToHost() { return &_rangeToHost; };

    inline DhtNode *getLocalNode() { return _spLocalNode.get(); };

    inline std::vector<DhtNode *> *getNeighbors(void) { return &_neighbors; };

    inline erpc::Nexus *getNexus() { return _spNexus.get(); }

    inline uint8_t getDhtThreadsCount() { return options.numOfDhtThreads; };

    void initNexus();

    /**
     * Gets DhtClient for caller thread.
     * Create new one if not initialized for the thread.
     *
     * @return DhtClient object
     */
    DhtClient *getClient();

    /**
     * Sets DhtClient for caller thread.
     */
    void setClient(DhtClient *dhtClient);

    /**
     * Storing each DhtClient is required to close Nexus gracefully (all
     * connected clients must be closed before that).
     *
     * @param dhtClient the dht client
     */
    void registerClient(DhtClient *dhtClient);

    DhtOptions options;
    std::atomic<int> nextRpcId;
    std::atomic<int> numberOfClientThreads;
    uint64_t randomSeed = 0;

  private:
    void _initNeighbors(void);
    void _initRangeToHost(void);
    void _initSeed(void);

    uint64_t _genHash(const char *key);

    /**
     * Separated DhtClient for each thread is required because of eRpc
     * architecture. eRpc 'endpoints' are created per thread.
     */
    static thread_local DhtClient *_threadDhtClient;
    std::vector<DhtClient *> _registeredDhtClients;
    /**
     * Needed to synchronize operations on _registeredDhtClients
     */
    std::mutex _dhtClientsMutex;

    std::unique_ptr<erpc::Nexus> _spNexus;
    std::unique_ptr<DhtNode> _spLocalNode;
    std::vector<DhtNode *> _neighbors;
    RangeToHost _rangeToHost;
    unsigned int _maskLength = 0;
    unsigned int _maskOffset = 0;
};

} // namespace DaqDB
