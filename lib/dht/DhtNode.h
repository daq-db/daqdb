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
     * @return Peer port number for this node
     */
    inline unsigned short getPeerPort() const { return _peerPort; };

    /**
     * @param portOffset offset that will be added to the node port
     * @return URI with IP and port for this node
     */
    inline const std::string getUri(unsigned int port = 0) const {
        return boost::str(boost::format("%1%:%2%") % getIp() %
                          std::to_string(port ? port : getPort()));
    };

    void setIp(const std::string &ip) { _ip = ip; };
    void setPort(unsigned short port) { _port = port; };
    void setPeerPort(unsigned short port) { _peerPort = port; };

    /**
     * @param id eRPC session id
     */
    void setSessionId(int id) { _sessionId = id; };

    inline unsigned int getStart() { return _start; }
    inline unsigned int getEnd() { return _end; }
    inline void setStart(unsigned int start) { _start = start; };
    inline void setEnd(unsigned int end) { _end = end; };

    std::atomic<DhtNodeState> state;

  private:
    std::string _ip = "";
    int _sessionId = ERPC_SESSION_NOT_SET;

    unsigned short _port = 0;
    /**
     * Second port is used for additional eRPC channel that is used for
     * communication between storage nodes.
     */
    unsigned short _peerPort = 0;

    // @TODO jradtke replace with other key mapping structures
    unsigned int _start = 0;
    unsigned int _end = 0;
};

} // namespace DaqDB
