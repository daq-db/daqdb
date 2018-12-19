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

#include <atomic>
#include <boost/format.hpp>
#include <string>

namespace DaqDB {

enum class DhtNodeState : std::uint8_t {
    NODE_INIT = 0,
    NODE_READY,
    NODE_NOT_RESPONDING
};

const int ERPC_SESSION_NOT_SET = -1;
const unsigned int ERPC_CLIENT_PORT_ADDITION = 32;

/*!
 * Class that defines interface for DHT
 */
class DhtNode {
  public:
    DhtNode();

    /**
     *  @return eRPC session id for this node
     */
    inline int getSessionId() const { return _sessionId; };

    /**
     * @return IP address for this node
     */
    inline const std::string &getIp() const { return _ip; };

    /**
     * @return Port number for this node
     */
    inline unsigned short getPort() const { return _port; };

    /**
     *
     * @return
     */
    inline const std::string getClientUri() const {
        return boost::str(
            boost::format("%1%:%2%") % getIp() %
            std::to_string(getPort() + ERPC_CLIENT_PORT_ADDITION));
    };
    /**
     *
     * @return
     */
    inline const std::string getUri() const {
        return boost::str(boost::format("%1%:%2%") % getIp() %
                          std::to_string(getPort()));
    };

    void setIp(const std::string &ip) { _ip = ip; };
    void setSessionId(int id) { _sessionId = id; };
    void setPort(unsigned short port) { _port = port; };

    inline unsigned int getMask() { return _mask; }
    inline unsigned int getStart() { return _start; }
    inline unsigned int getEnd() { return _end; }
    inline void setMask(unsigned int mask) { _mask = mask; };
    inline void setStart(unsigned int start) { _start = start; };
    inline void setEnd(unsigned int end) { _end = end; };

    std::atomic<DhtNodeState> state;

  private:
    std::string _ip = "";
    int _sessionId = ERPC_SESSION_NOT_SET;
    unsigned short _port = 0;

    // @TODO jradtke replace with other key mapping structures
    unsigned int _mask = 0;
    unsigned int _start = 0;
    unsigned int _end = 0;
};

} // namespace DaqDB
