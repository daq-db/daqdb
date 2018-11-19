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

#include <daqdb/KVStoreBase.h>
#include <iostream>
#include <linenoise.h>
#include <memory>

#include "rpc.h"
#include "config.h"

namespace {
const unsigned int consoleHintColor = 35; // dark red
};

namespace DaqDB {
	void erpcReqHandler(erpc::ReqHandle *req_handle, void *);

/*!
 * Dragon shell interpreter.
 * Created for test purposes - to allow performing quick testing of the node.
 */
class nodeCli {
  public:
    explicit nodeCli(std::shared_ptr<DaqDB::KVStoreBase> &spDragonSrv);
    virtual ~nodeCli();

    /*!
     * Waiting for user input, executes defined commands
     *
     * @return false if user choose "quit" command, otherwise true
     */
    int operator()();

  private:
    void _cmdGet(const std::string &strLine);
    void _cmdGetAsync(const std::string &strLine);
    void _cmdPut(const std::string &strLine);
    void _cmdPutAsync(const std::string &strLine);
    void _cmdUpdate(const std::string &strLine);
    void _cmdUpdateAsync(const std::string &strLine);
    void _cmdRemove(const std::string &strLine);
    void _cmdStatus();
    void _cmdNeighbors();

    DaqDB::Key _strToKey(const std::string &key);
    DaqDB::Value _strToValue(const std::string &valStr);
    DaqDB::Value _allocValue(const DaqDB::Key &key, const std::string &valStr);

    DaqDB::PrimaryKeyAttribute
    _getKeyAttrs(unsigned char start, const std::vector<std::string> &cmdAttrs);
    DaqDB::PrimaryKeyAttribute
    _getKeyAttr(unsigned char start, const std::vector<std::string> &cmdAttrs);

    std::shared_ptr<DaqDB::KVStoreBase> _spKVStore;
    std::vector<std::string> _statusMsgs;
};
}
