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

static const unsigned int TESTCLIENT_PORT = 31852;

bool testDhtConnect(KVStoreBase *kvs) {
    bool result = true;

    DhtOptions options;
    DhtNeighbor local;
    local.ip = "localhost";
    local.port = TESTCLIENT_PORT;
    local.local = true;
    options.maskLength = 8;
    options.maskOffset = 0;

    DhtNeighbor neighbor;
    neighbor.ip = "localhost";
    neighbor.port = 31850;
    neighbor.keyRange.start = "0";
    neighbor.keyRange.end = "255";

    options.neighbors.push_back(&local);
    options.neighbors.push_back(&neighbor);

    auto serverStatus = kvs->getProperty("daqdb.dht.status");
    if (serverStatus.find("DHT server: inactive") != std::string::npos) {
        DAQDB_INFO << "DHT server not started" << flush;
        return false;
    }

    auto core = new DhtCore(options);
    core->initNexus();
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

    return result;
}
