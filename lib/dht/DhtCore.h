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
#include <vector>

#include <DhtClient.h>
#include <DhtNode.h>

#include <daqdb/Key.h>
#include <daqdb/Options.h>

// @TODO : jradtke Cannot include rpc.h, net / if.h conflict
namespace erpc {
class Nexus;
}

namespace DaqDB {

typedef std::map<std::pair<int, int>, DhtNode *> RangeToHost;

enum class ErpRequestType : std::uint8_t {
    ERP_REQUEST_GET = 2,
    ERP_REQUEST_PUT,
    ERP_REQUEST_REMOVE
};

struct DaqdbDhtMsg {
    size_t keySize;
    size_t valSize;
    // Contains both key and value
    char msg[];
};

struct DaqdbDhtResult {
    StatusCode status;
    size_t msgSize;
    // Contains value for get requests
    char msg[];
};

class DhtCore {
  public:
    DhtCore(DhtOptions dhtOptions);
    ~DhtCore();

    void initClient();

    bool isLocalKey(Key key);

    DhtNode *getHostForKey(Key key);

    inline RangeToHost *getRangeToHost() { return &_rangeToHost; };

    inline DhtNode *getLocalNode() { return _spLocalNode.get(); };

    inline std::vector<DhtNode *> *getNeighbors(void) { return &_neighbors; };

    inline DhtClient *getClient() { return _spClient.get(); };

    DhtOptions options;

  private:
    void _initNeighbors(void);
    void _initRangeToHost(void);

    uint64_t _genHash(const char *key, uint64_t maskLength,
                      uint64_t maskOffset);

    std::unique_ptr<DhtClient> _spClient;

    std::unique_ptr<DhtNode> _spLocalNode;
    std::vector<DhtNode *> _neighbors;
    RangeToHost _rangeToHost;
};

} // namespace DaqDB
