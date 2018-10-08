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

#include <thread>

#include <Options.h>
#include <Value.h>
#include <Key.h>

#include <DhtNode.h>
#include "DhtNodeInfo.h"
#include "DhtClient.h"
#include <ZhtClient.h>

namespace DaqDB {

class ZhtNode : public DaqDB::DhtNode {
  public:
    ZhtNode(asio::io_service &io_service, Options options, unsigned short port,
            const std::string &confFile = "",
            const std::string &neighborsFile = "");
    virtual ~ZhtNode();

    /*!
     * @return Status of the Node - format determined by 3rd party lib
     */
    std::string printStatus();

    /*!
     * Prints DHT neighbors.
     * @return
     */
    virtual std::string printNeighbors();

    virtual Value Get(const Key &key);

  private:
    void _ThreadMain(void);
    void _initNeighbors(void);

    std::thread *_thread;

    DhtClient<ZHTClient> _client;
    map<PureNode*, DhtNodeInfo*> _neighbors;

    std::string _confFile;
    std::string _neighborsFile;
    Options _options;
};

} // namespace DaqDB
