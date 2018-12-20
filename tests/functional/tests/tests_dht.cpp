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

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

#include <DhtClient.h>
#include <DhtCore.h>
#include <Options.h>

#include "tests.h"
#include <config.h>
#include <debug.h>

using namespace std;
using namespace DaqDB;

bool testDhtConnect(KVStoreBase *kvs) {
    bool result = true;

    DhtOptions options;
    DhtNeighbor local;
    local.ip = "localhost";
    local.port = 31851;
    local.local = true;
    local.keyRange.mask = "1";

    DhtNeighbor neighbor;
    neighbor.ip = "localhost";
    neighbor.port = 31850;
    neighbor.keyRange.start = "0";
    neighbor.keyRange.end = "255";
    neighbor.keyRange.mask = "1";

    options.neighbors.push_back(&local);
    options.neighbors.push_back(&neighbor);

    auto core = new DhtCore(options);
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
