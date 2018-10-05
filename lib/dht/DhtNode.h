/**
 * Copyright 2017-2018 Intel Corporation.
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

#include <Value.h>
#include <Key.h>
#include "PureNode.h"
#include <asio/io_service.hpp>

namespace DaqDB {

/*!
 * Class that defines interface for DHT
 */
class DhtNode : public PureNode {
  public:
    DhtNode(asio::io_service &io_service, unsigned short port);
    virtual ~DhtNode();

    /*!
     * Prints DHT status.
     * @return
     */
    virtual std::string printStatus() = 0;

    /*!
     * Prints DHT neighbors.
     * @return
     */
    virtual std::string printNeighbors() = 0;

    virtual Value Get(const Key &key) = 0;
};

} // namespace DaqDB
