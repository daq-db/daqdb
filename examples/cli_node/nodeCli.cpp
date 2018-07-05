/**
 * Copyright 2017, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of the copyright holder nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "../cli_node/nodeCli.h"
#include <FogKV/Types.h>
#include <FogKV/Options.h>

#include <boost/algorithm/string.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <chrono>
#include <iostream>
#include <map>

using namespace std;
using namespace boost::algorithm;
using boost::format;

#define NUM_ELEM_KEY_ATTRS   3
#define UPDATE_CMD_KEY_ATTRS_OFFSET   3
#define UPDATE_CMD_VAL_OFFSET   2
#define KEY_ATTRS_OPT_NAME_POS_OFFSET   1
#define KEY_ATTRS_OPT_VAL_POS_OFFSET   2

map<string, string> consoleCmd =
    boost::assign::map_list_of("help", "")("get", " <key>")("aget", " <key>")(
        "put", " <key> <value>")("aput", " <key> <value>")("status", "")(
        "remove", " <key>")("quit", "")("node", " <id>")(
        "update", " <key> [value] [-o <lock|ready|long_term> <0|1>]")(
        "aupdate", " <key> [value] [-o <lock|ready|long_term> <0|1>]");

/*!
 * Provides completion functionality to dragon shell.
 *
 * @param buf	completion prefix
 * @param lc	callback functions manager
 */
void completion(const char *buf, linenoiseCompletions *lc) {
    if (buf[0] == 'h') {
        linenoiseAddCompletion(lc, "help");
    } else if (buf[0] == 'q') {
        linenoiseAddCompletion(lc, "quit");
    } else if (buf[0] == 'g') {
        linenoiseAddCompletion(lc, "get");
    } else if (buf[0] == 'u') {
        linenoiseAddCompletion(lc, "update");
    } else if (buf[0] == 'p') {
        linenoiseAddCompletion(lc, "put");
    } else if (buf[0] == 's') {
        linenoiseAddCompletion(lc, "status");
    } else if (buf[0] == 'n') {
        linenoiseAddCompletion(lc, "node");
    } else if (buf[0] == 'r') {
        linenoiseAddCompletion(lc, "remove");
    } else if (buf[0] == 'a') {
        if (buf[1] == 'g') {
            linenoiseAddCompletion(lc, "aget");
        } else if (buf[1] == 'p') {
            linenoiseAddCompletion(lc, "aput");
        } else if (buf[1] == 'u') {
            linenoiseAddCompletion(lc, "aupdate");
        }
    }
}

/*!
 * Provides hints for commands to dragon shell.
 *
 * @param buf	command
 * @param color	hint color
 * @param bold	indicate if hint should be bolded
 * @return
 */
char *hints(const char *buf, int *color, int *bold) {
    *color = consoleHintColor;
    char *result = nullptr;

    if (consoleCmd.count(buf)) {
        result = const_cast<char *>(consoleCmd[buf].c_str());
    }

    return result;
}

namespace FogKV {

nodeCli::nodeCli(std::shared_ptr<FogKV::KVStoreBase> &spKVStore)
    : _spKVStore(spKVStore) {
    linenoiseSetCompletionCallback(completion);
    linenoiseSetHintsCallback(hints);
}

nodeCli::~nodeCli() {}

int nodeCli::operator()() {
    auto *inLine = linenoise("fogkv> ");
    auto isEmpty = false;
    if (inLine == nullptr) {
        return false;
    }
    if (inLine[0] == '\0') {
        isEmpty = true;
    }
    std::string strLine(inLine);
    free(inLine);

    try {
        if (isEmpty || starts_with(strLine, "help")) {
            cout << "Following commands supported:" << endl;
            for (auto cmdEntry : consoleCmd) {
                cout << "\t- " << cmdEntry.first << cmdEntry.second << endl;
            }
        } else if (starts_with(strLine, "g")) {
            this->cmdGet(strLine);
        } else if (starts_with(strLine, "p")) {
            this->cmdPut(strLine);
        } else if (starts_with(strLine, "u")) {
            this->cmdUpdate(strLine);
        } else if (starts_with(strLine, "ap")) {
            this->cmdPutAsync(strLine);
        } else if (starts_with(strLine, "ag")) {
            this->cmdGetAsync(strLine);
        } else if (starts_with(strLine, "au")) {
            this->cmdUpdateAsync(strLine);
        } else if (starts_with(strLine, "r")) {
            this->cmdRemove(strLine);
        } else if (starts_with(strLine, "s")) {
            this->cmdStatus();
        } else if (starts_with(strLine, "n")) {
            this->cmdNodeStatus(strLine);
        } else if (starts_with(strLine, "q")) {
            return false;
        } else {
            cout << format("Unreconized command: %1%\n") % strLine;
        }
    } catch (NotImplementedException &ex) {
        cout << ex.what() << endl;
    }

    return true;
}

void nodeCli::cmdGet(const std::string &strLine) {
    vector<string> arguments;
    split(arguments, strLine, is_any_of("\t "), boost::token_compress_on);

    if (arguments.size() != 2) {
        cout << "Error: expects one argument" << endl;
    } else {
        auto key = arguments[1];
        if (key.size() > _spKVStore->KeySize()) {
            cout << "Error: key size is " << _spKVStore->KeySize() << endl;
            return;
        }
        FogKV::Key keyBuff;
        try {
            keyBuff = _spKVStore->AllocKey();
            /** @todo memleak - keyBuff.data */
            std::memset(keyBuff.data(), 0, keyBuff.size());
            std::memcpy(keyBuff.data(), key.c_str(), key.size());
        } catch (FogKV::OperationFailedException &e) {
            cout << "Error: cannot allocate key buffer" << endl;
            return;
        }

        FogKV::Value value;
        try {
            value = _spKVStore->Get(keyBuff);
            string valuestr(value.data());
            cout << format("[%1%] = %2%\n") % key % valuestr;
        } catch (FogKV::OperationFailedException &e) {
            if (e.status()() == FogKV::KeyNotFound) {
                cout << format("[%1%] not found\n") % key;
            } else {
                cout << "Error: cannot get element: " << e.status().to_string()
                     << endl;
            }
        }
        if (value.size() > 0)
            _spKVStore->Free(std::move(value));
        if (keyBuff.size() > 0)
            _spKVStore->Free(std::move(keyBuff));
    }
}

void nodeCli::cmdGetAsync(const std::string &strLine) {
    vector<string> arguments;
    split(arguments, strLine, is_any_of("\t "), boost::token_compress_on);

    if (arguments.size() != 2) {
        cout << "Error: expects one argument" << endl;
    } else {
        auto key = arguments[1];
        if (key.size() > _spKVStore->KeySize()) {
            cout << "Error: key size is " << _spKVStore->KeySize() << endl;
            return;
        }
        FogKV::Key keyBuff;
        try {
            keyBuff = _spKVStore->AllocKey();
            std::memset(keyBuff.data(), 0, keyBuff.size());
            std::memcpy(keyBuff.data(), key.c_str(), key.size());
        } catch (FogKV::OperationFailedException &e) {
            cout << "Error: cannot allocate key buffer" << endl;
            return;
        }

        _spKVStore->GetAsync(
            std::move(keyBuff),
            [&](KVStoreBase *kvs, Status status, const char *key,
                size_t keySize, const char *value, size_t valueSize) {
                if (!status.ok()) {
                    _statusMsgs.push_back(boost::str(
                        boost::format("Error: cannot get element: %1%") %
                        status.to_string()));
                } else {
                    _statusMsgs.push_back(
                        boost::str(boost::format("GET[%1%]=%2% : completed") %
                                   key % value));
                }
                if (valueSize > 0 && value != nullptr)
                    delete[] value;
                if (keyBuff.size() > 0)
                    _spKVStore->Free(std::move(keyBuff));
            });
    }
}

FogKV::Key nodeCli::strToKey(const std::string &key) {
    FogKV::Key keyBuff = _spKVStore->AllocKey();
    std::memset(keyBuff.data(), 0, keyBuff.size());
    std::memcpy(keyBuff.data(), key.c_str(), key.size());
    return keyBuff;
}

FogKV::PrimaryKeyAttribute
nodeCli::_getKeyAttr(unsigned char start,
                     const std::vector<std::string> &cmdAttrs) {
    if ((cmdAttrs.size() < start + NUM_ELEM_KEY_ATTRS) ||
        !(starts_with(cmdAttrs[start], "-o"))) {
        return FogKV::PrimaryKeyAttribute::EMPTY;
    }

    auto optVal = boost::lexical_cast<bool>(
        cmdAttrs[start + KEY_ATTRS_OPT_VAL_POS_OFFSET]);
    if (optVal) {
        string optName = cmdAttrs[start + KEY_ATTRS_OPT_NAME_POS_OFFSET];
        if (optName == "lock") {
            return FogKV::PrimaryKeyAttribute::LOCKED;
        } else if (optName == "ready") {
            return FogKV::PrimaryKeyAttribute::READY;
        } else if (optName == "long_term") {
            return FogKV::PrimaryKeyAttribute::LONG_TERM;
        }
    }
    return FogKV::PrimaryKeyAttribute::EMPTY;
}

FogKV::PrimaryKeyAttribute
nodeCli::_getKeyAttrs(unsigned char start,
                      const std::vector<std::string> &cmdAttrs) {
    if (cmdAttrs.size() < start + NUM_ELEM_KEY_ATTRS) {
        return FogKV::PrimaryKeyAttribute::EMPTY;
    }
    if ((cmdAttrs.size() - start) % NUM_ELEM_KEY_ATTRS != 0) {
        cout << "Warning: cannot parse key attributes" << endl;
        return FogKV::PrimaryKeyAttribute::EMPTY;
    }

    unsigned char parsingPositon = start;
    FogKV::PrimaryKeyAttribute result = FogKV::PrimaryKeyAttribute::EMPTY;
    while (parsingPositon <= cmdAttrs.size()) {
        result = result | _getKeyAttr(parsingPositon, cmdAttrs);
        parsingPositon += NUM_ELEM_KEY_ATTRS;
    }

    return result;
}

void nodeCli::cmdPut(const std::string &strLine) {
    vector<string> arguments;
    split(arguments, strLine, is_any_of("\t "), boost::token_compress_on);

    if (arguments.size() != 3) {
        cout << "Error: expects two arguments" << endl;
    } else {
        auto key = arguments[1];
        if (key.size() > _spKVStore->KeySize()) {
            cout << "Error: key size is " << _spKVStore->KeySize() << endl;
            return;
        }

        FogKV::Key keyBuff;
        try {
            keyBuff = strToKey(key);
        } catch (...) {
            cout << "Error: cannot allocate key buffer" << endl;
            return;
        }

        auto value = arguments[2];

        FogKV::Value valBuff;
        try {
            valBuff = _spKVStore->Alloc(keyBuff, value.size() + 1);
            std::memcpy(valBuff.data(), value.c_str(), value.size());
            valBuff.data()[valBuff.size() - 1] = '\0';
        } catch (...) {
            cout << "Error: cannot allocate value buffer" << endl;
            _spKVStore->Free(std::move(keyBuff));
            return;
        }

        try {
            _spKVStore->Put(std::move(keyBuff), std::move(valBuff));
            cout << format("Put: [%1%] = %2%\n") % key % value;
        } catch (FogKV::OperationFailedException &e) {
            cout << "Error: cannot put element: " << e.status().to_string()
                 << endl;

            _spKVStore->Free(std::move(keyBuff));
            // @TODO jschmieg: free value
            //_spKVStore->Free(std::move(valBuff));
        }

        if (keyBuff.size() > 0)
            _spKVStore->Free(std::move(keyBuff));
    }
}

FogKV::Value nodeCli::strToValue(const std::string &val) {
    Value result;
    if (!starts_with(val, "-o")) {
        auto buffer = new char[val.size() + 1];
        result = Value(buffer, val.size() + 1);
        std::memcpy(result.data(), val.c_str(), val.size());
        result.data()[result.size() - 1] = '\0';
    }
    return result;
}

void nodeCli::cmdUpdate(const std::string &strLine) {
    vector<string> arguments;
    split(arguments, strLine, is_any_of("\t "), boost::token_compress_on);

    if (arguments.size() < 2) {
        cout << "Error: expects at least one argument" << endl;
    } else {
        auto key = arguments[1];
        if (key.size() > _spKVStore->KeySize()) {
            cout << "Error: key size is " << _spKVStore->KeySize() << endl;
            return;
        }
        FogKV::Key keyBuff;
        try {
            keyBuff = strToKey(key);
        } catch (...) {
            cout << "Error: cannot allocate key buffer" << endl;
            return;
        }

        FogKV::Value valBuff;
        unsigned short optionStartPos = UPDATE_CMD_KEY_ATTRS_OFFSET;
        if (!starts_with(arguments[UPDATE_CMD_VAL_OFFSET], "-o")) {
            valBuff = strToValue(arguments[UPDATE_CMD_VAL_OFFSET]);
        } else {
            optionStartPos = UPDATE_CMD_VAL_OFFSET;
        }

        auto keyAttrs = _getKeyAttrs(optionStartPos, arguments);
        FogKV::UpdateOptions options(keyAttrs);

        try {
            if (valBuff.size() > 0) {
                _spKVStore->Update(std::move(keyBuff), std::move(valBuff),
                                   std::move(options));
            } else {
                _spKVStore->Update(std::move(keyBuff), std::move(options));
            }
        } catch (FogKV::OperationFailedException &e) {
            cout << "Error: cannot update element: " << e.status().to_string()
                 << endl;
        }
    }
}

void nodeCli::cmdUpdateAsync(const std::string &strLine) {
    vector<string> arguments;
    split(arguments, strLine, is_any_of("\t "), boost::token_compress_on);

    if (arguments.size() < 2) {
        cout << "Error: expects at least one argument" << endl;
    } else {
        auto key = arguments[1];
        if (key.size() > _spKVStore->KeySize()) {
            cout << "Error: key size is " << _spKVStore->KeySize() << endl;
            return;
        }
        FogKV::Key keyBuff;
        try {
            keyBuff = strToKey(key);
        } catch (...) {
            cout << "Error: cannot allocate key buffer" << endl;
            return;
        }

        FogKV::Value valBuff;
        unsigned short optionStartPos = UPDATE_CMD_KEY_ATTRS_OFFSET;
        if (!starts_with(arguments[UPDATE_CMD_VAL_OFFSET], "-o")) {
            valBuff = strToValue(arguments[UPDATE_CMD_VAL_OFFSET]);
        } else {
            optionStartPos = UPDATE_CMD_VAL_OFFSET;
        }

        auto keyAttrs = _getKeyAttrs(optionStartPos, arguments);
        FogKV::UpdateOptions options(keyAttrs);

        try {
            if (valBuff.size() > 0) {
                _spKVStore->UpdateAsync(
                    std::move(keyBuff), std::move(valBuff),
                    [&](FogKV::KVStoreBase *kvs, FogKV::Status status,
                        const char *key, const size_t keySize,
                        const char *value, const size_t valueSize) {
                        if (!status.ok()) {
                            _statusMsgs.push_back(boost::str(
                                boost::format(
                                    "Error: cannot update element: %1%") %
                                status.to_string()));
                        } else {
                            _statusMsgs.push_back(boost::str(
                                boost::format("UPDATE[%1%]=%2% (Opts:%3%) : completed") %
                                key % value % options.Attr));
                        }
                        if (keyBuff.size() > 0)
                            _spKVStore->Free(std::move(keyBuff));
                    }, std::move(options));
            } else {
                _spKVStore->UpdateAsync(
                    std::move(keyBuff), std::move(options),
                    [&](FogKV::KVStoreBase *kvs, FogKV::Status status,
                        const char *key, const size_t keySize,
                        const char *value, const size_t valueSize) {
                        if (!status.ok()) {
                            _statusMsgs.push_back(boost::str(
                                boost::format(
                                    "Error: cannot update element: %1%") %
                                status.to_string()));
                        } else {
                            _statusMsgs.push_back(boost::str(
                                boost::format("UPDATE[%1%]=%2% (Opts:%3%) : completed") %
                                key % value % options.Attr));
                        }
                        if (keyBuff.size() > 0)
                            _spKVStore->Free(std::move(keyBuff));
                    });
            }
        } catch (FogKV::OperationFailedException &e) {
            cout << "Error: cannot update element: " << e.status().to_string()
                 << endl;
        }
    }
}

void nodeCli::cmdPutAsync(const std::string &strLine) {
    vector<string> arguments;
    split(arguments, strLine, is_any_of("\t "), boost::token_compress_on);

    if (arguments.size() != 3) {
        cout << "Error: expects two arguments" << endl;
    } else {
        auto key = arguments[1];
        if (key.size() > _spKVStore->KeySize()) {
            cout << "Error: key size is " << _spKVStore->KeySize() << endl;
            return;
        }

        FogKV::Key keyBuff;
        try {
            keyBuff = strToKey(key);
        } catch (...) {
            cout << "Error: cannot allocate key buffer" << endl;
            return;
        }

        auto value = arguments[2];

        FogKV::Value valBuff;
        try {
            valBuff = _spKVStore->Alloc(keyBuff, value.size() + 1);
            std::memcpy(valBuff.data(), value.c_str(), value.size());
            valBuff.data()[valBuff.size() - 1] = '\0';
        } catch (...) {
            cout << "Error: cannot allocate value buffer" << endl;
            _spKVStore->Free(std::move(keyBuff));
            return;
        }

        try {
            _spKVStore->PutAsync(
                std::move(keyBuff), std::move(valBuff),
                [&](FogKV::KVStoreBase *kvs, FogKV::Status status,
                    const char *key, const size_t keySize, const char *value,
                    const size_t valueSize) {
                    if (!status.ok()) {
                        _statusMsgs.push_back(boost::str(
                            boost::format("Error: cannot put element: %1%") %
                            status.to_string()));
                    } else {
                        _statusMsgs.push_back(boost::str(
                            boost::format("PUT[%1%]=%2% : completed") % key %
                            value));
                    }
                    if (keyBuff.size() > 0)
                        _spKVStore->Free(std::move(keyBuff));
                });
        } catch (FogKV::OperationFailedException &e) {
            cout << "Error: cannot put element: " << e.status().to_string()
                 << endl;

            if (keyBuff.size() > 0)
                _spKVStore->Free(std::move(keyBuff));

            // @TODO jschmieg: free value
            // _spKVStore->Free(std::move(valBuff));
        }
    }
}

void nodeCli::cmdRemove(const std::string &strLine) {
    vector<string> arguments;
    split(arguments, strLine, is_any_of("\t "), boost::token_compress_on);

    if (arguments.size() != 2) {
        cout << "Error: expects one argument" << endl;
    } else {

        auto key = arguments[1];
        FogKV::Key keyBuff;
        try {
            keyBuff = strToKey(key);
        } catch (...) {
            cout << "Error: cannot allocate key buffer" << endl;
            return;
        }

        try {
            _spKVStore->Remove(keyBuff);
            cout << format("Remove: [%1%]\n") % key;
        } catch (FogKV::OperationFailedException &e) {
            if (e.status()() == FogKV::KeyNotFound) {
                cout << format("[%1%] not found\n") % key;
            } else {
                cout << "Error: cannot remove element" << endl;
            }
        }
    }
}

Json::Value nodeCli::getPeersJson() {
    Json::Reader reader;
    Json::Value peers;
    std::string peersStr = _spKVStore->getProperty("fogkv.dht.peers");
    reader.parse(peersStr, peers);

    return peers;
}

void nodeCli::cmdStatus() {
    Json::Value peers = getPeersJson();

    auto npeers = peers.size();
    chrono::time_point<chrono::system_clock> timestamp;
    auto currentTime = chrono::system_clock::to_time_t(timestamp);

    if (!npeers) {
        cout << format("%1%\tno DHT peer(s)\n") % std::ctime(&currentTime);
    } else {
        cout << format("%1%\t%2% DHT "
                       "peers found") %
                    std::ctime(&currentTime) % npeers
             << endl;

        for (int i = 0; i < npeers; i++) {
            cout << "[" << i << "]: " << peers[i].toStyledString();
        }
    }

    if (_statusMsgs.size() > 0) {
        cout << "Async operations statuses:" << endl;
        for (std::string message : _statusMsgs) {
            cout << "\t- " << message << endl;
        }
        _statusMsgs.clear();
    }
}

void nodeCli::cmdNodeStatus(const std::string &strLine) {
    vector<string> arguments;
    split(arguments, strLine, is_any_of("\t "), boost::token_compress_on);

    if (arguments.size() != 2) {
        cout << "Error: expects one argument" << endl;
    } else {
        auto nodeId = arguments[1];

        if (nodeId == _spKVStore->getProperty("fogkv.dht.id")) {
            cout << format("Node status:\n\t "
                           "dht_id=%1%\n\tdht_ip:port=%2%:%3%"
                           "\n\texternal_port=%4%") %
                        _spKVStore->getProperty("fogkv.dht.id") %
                        _spKVStore->getProperty("fogkv.listener.ip") %
                        _spKVStore->getProperty("fogkv.listener.port") %
                        _spKVStore->getProperty("fogkv.listener.dht_port")
                 << endl;
            return;
        }

        Json::Value peers = getPeersJson();

        for (int i = 0; i < peers.size(); i++) {
            if (peers[i]["id"].asString() == nodeId) {
                cout << peers[i].toStyledString() << endl;
                return;
            }
        }

        cout << "Node not connected" << endl;
    }
}
}
