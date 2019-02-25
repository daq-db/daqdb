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

#include <daqdb/KVStoreBase.h>
#include <iostream>
#include <linenoise.h>
#include <memory>

namespace {
const unsigned int consoleHintColor = 35; // dark red
};

namespace DaqDB {

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

    /**
     * @return true if key is NOT correct
     */
    inline bool _isInvalidKey(const std::string &key) {
        return ((key.size() == 0) ||
                (key.size() > sizeof(CliNodeKey::eventId)));
    }

    DaqDB::PrimaryKeyAttribute
    _getKeyAttrs(unsigned char start, const std::vector<std::string> &cmdAttrs);
    DaqDB::PrimaryKeyAttribute
    _getKeyAttr(unsigned char start, const std::vector<std::string> &cmdAttrs);

    std::shared_ptr<DaqDB::KVStoreBase> _spKVStore;
    std::vector<std::string> _statusMsgs;
};
} // namespace DaqDB
