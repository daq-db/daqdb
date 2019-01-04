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
            DhtNeighbor local;
            local.ip = node->ip;
            local.keyRange.maskLength = 8;
            local.keyRange.maskOffset = 0;
            local.port = node->port + 1;
            local.local = true;
            clientOptions.neighbors.push_back(&local);
        } else {
            clientOptions.neighbors.push_back(node);
        }
    }

    auto core = new DhtCore(clientOptions);
    core->initClient();
    if (core->getClient()->state == DhtClientState::DHT_CLIENT_READY) {
        LOG_INFO << "DHT client started successfully" << flush;
    } else {
        LOG_INFO << "Can not start DHT client" << flush;
        return false;
    }
    auto neighborNode = core->getNeighbors()->front();
    auto connected = core->getClient()->ping(*neighborNode);
    if (!connected) {
        LOG_INFO << "Can not connect to neighbor" << flush;
        return false;
    }

    return result;
}

bool testPutGetSequence(KVStoreBase *kvs, DaqDB::Options *options) {
    bool result = true;

    const string expectedVal = "daqdb";
    const string expectedKey = "1000";

    auto key = strToKey(kvs, expectedKey);
    auto val = allocValue(kvs, key, expectedVal);

    remote_put(kvs, key, val);
    LOG_INFO << format("Remote Put: [%1%] = %2%") % key.data() % val.data();

    auto resultVal = remote_get(kvs, key);
    if (resultVal.data()) {
        LOG_INFO << format("Remote Get result: [%1%] = %2%") % key.data() %
                        resultVal.data();
    }

    if (!resultVal.data() || expectedVal.compare(resultVal.data()) != 0) {
        result = false;
        LOG_INFO << "Error: wrong value returned";
    }

    auto removeResult = remote_remove(kvs, key);
    LOG_INFO << format("Remote Remove: [%1%]") % key.data();
    if (!removeResult) {
        result = false;
        LOG_INFO << format("Error: Cannot remove a key [%1%]") % key.data();
    }

    return result;
}
