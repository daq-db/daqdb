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

#include <string>
#include <vector>

#include <Util.h>
#include <ZhtClient.h>

#include <DhtClient.h>
#include <DhtNode.h>
#include <daqdb/Options.h>

namespace DaqDB {

typedef std::map<std::pair<int, int>, int> RangeToHost;

const std::string DEFAULT_ZHT_CONF_FILE = "zht.conf";
const std::string DEFAULT_ZHT_NEIGHBORS_FILE = "neighbors.conf";
const size_t DEFAULT_ZHT_MSG_MAXSIZE = 1000000L;
const unsigned int DEFAULT_ZHT_SCCB_POOL_INTERVAL = 100;
const unsigned int DEFAULT_ZHT_INSTANT_SWAP = 0;

class DhtCore {
  public:
    DhtCore(DhtOptions dhtOptions);

    void createConfFile(void);

    void createNeighborsFile(void);

    void removeConfFile(void);

    void removeNeighborsFile(void);

    bool isLocalKey(Key key);

    inline DhtClient<ZHTClient> *getClient() { return &_zhtClient; };
    inline RangeToHost *getRangeToHost() { return &_rangeToHost; }
    inline DhtNode *getLocalNode() { return _spLocalNode.get(); }
    inline vector<DhtNode *> *getNeighbors(void) { return &_neighbors; }

    DhtOptions options;

  private:
    void _initNeighbors(void);
    void _initRangeToHost(void);
    bool _isLocalServerNode(string ip, string port, unsigned short serverPort);

    uint64_t _genHash(const char *key, int mask);

    std::unique_ptr<DhtNode> _spLocalNode;
    vector<DhtNode *> _neighbors;
    RangeToHost _rangeToHost;

    DhtClient<ZHTClient> _zhtClient;
};

} // namespace DaqDB
