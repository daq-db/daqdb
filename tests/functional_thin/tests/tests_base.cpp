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

#include "boost/algorithm/string.hpp"

#include "common.h"
#include "tests.h"

#include <DhtClient.h>
#include <DhtCore.h>

using namespace std;
using namespace DaqDB;

bool testRemotePeerConnect(KVStoreBase *kvs, DaqDB::Options *options) {
    bool result = true;

    DhtOptions clientOptions;
    for (auto node : options->dht.neighbors) {
        if (node->local) {
            auto local = new DhtNeighbor();
            local->ip = node->ip;
            local->keyRange.maskLength = 1;
            local->keyRange.maskOffset = 0;
            local->port = node->port + 1;
            local->local = true;
            clientOptions.neighbors.push_back(local);
        } else {
            clientOptions.neighbors.push_back(node);
        }
    }

    auto core = new DhtCore(clientOptions);
    core->initNexus(core->getLocalNode()->getPort());
    core->initClient();
    if (core->getClient()->state == DhtClientState::DHT_CLIENT_READY) {
        DAQDB_INFO << "DHT client started successfully" << flush;
    } else {
        DAQDB_INFO << "Can not start DHT client" << flush;
        return false;
    }
    auto neighborNode = core->getNeighbors()->front();
    auto connected = core->getClient()->ping(*neighborNode);
    if (!connected) {
        DAQDB_INFO << "Can not connect to neighbor" << flush;
        return false;
    }

    for (auto node : clientOptions.neighbors)
        if (node->local)
            delete node;

    return result;
}

bool testPutGetSequence(KVStoreBase *kvs, DaqDB::Options *options) {
    bool result = true;

    const string expectedVal = "daqdb";
    const string expectedKey = "1000";

    auto putKey = strToKey(kvs, expectedKey);
    auto val = allocValue(kvs, putKey, expectedVal);

    DAQDB_INFO << format("Remote Put: [%1%] = %2%") % putKey.data() %
                      val.data();
    remote_put(kvs, move(putKey), val);

    auto key = strToKey(kvs, expectedKey, KeyValAttribute::NOT_BUFFERED);
    auto resultVal = remote_get(kvs, key);
    if (resultVal.data()) {
        DAQDB_INFO << format("Remote Get result: [%1%] = %2%") % key.data() %
                          resultVal.data();
        if (expectedVal.compare(resultVal.data()) != 0) {
            result = false;
            DAQDB_INFO << "Error: wrong value returned";
        }
    } else {
        result = false;
        DAQDB_INFO << "Error: no value returned";
    }

    auto removeResult = remote_remove(kvs, key);
    DAQDB_INFO << format("Remote Remove: [%1%]") % key.data();
    if (!removeResult) {
        result = false;
        DAQDB_INFO << format("Error: Cannot remove a key [%1%]") % key.data();
    }

    return result;
}
