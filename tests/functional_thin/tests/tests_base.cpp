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
