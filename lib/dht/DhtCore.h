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
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include <DhtClient.h>
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
     * @param dhtOptions dht options
     *
     * @note At the moment only satellite node should initialize nexus
     */
    DhtCore(DhtOptions dhtOptions);
    ~DhtCore();

    void initClient();

    bool isLocalKey(Key key);

    DhtNode *getHostForKey(Key key);

    inline RangeToHost *getRangeToHost() { return &_rangeToHost; };

    inline DhtNode *getLocalNode() { return _spLocalNode.get(); };

    inline std::vector<DhtNode *> *getNeighbors(void) { return &_neighbors; };

    inline erpc::Nexus *getNexus() { return _spNexus.get(); }

    void initNexus(unsigned int portOffset = 0);

    /**
     * Gets DhtClient for caller thread.
     * Create new one if not initialized for the thread.
     *
     * @return DhtClient object
     */
    inline DhtClient *getClient() {
        if (!_threadDhtClient) {
            /*
             * Separate DHT client is needed per user thread.
             * It is expected that on first use the DhtClient have to be created
             * and initialized.
             */
            initClient();
        }

        return _threadDhtClient;
    };

    /**
     * Storing each DhtClient is required to close Nexus gracefully (all
     * connected clients must be closed before that).
     *
     * @param dhtClient the dht client
     */
    void registerClient(DhtClient *dhtClient);

    DhtOptions options;
    std::atomic<int> numberOfClients;

  private:
    void _initNeighbors(void);
    void _initRangeToHost(void);

    uint64_t _genHash(const char *key, uint64_t maskLength,
                      uint64_t maskOffset);

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
};

} // namespace DaqDB
