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

#include <daqdb/Types.h>

#include <boost/algorithm/string.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <chrono>
#include <iostream>
#include <map>

#include "nodeCli.h"

using namespace std;
using namespace boost::algorithm;
using boost::format;

#define NUM_ELEM_KEY_ATTRS 3
#define GET_CMD_KEY_ATTRS_OFFSET 2
#define PUT_CMD_KEY_ATTRS_OFFSET 3
#define PUT_CMD_VAL_OFFSET 2
#define UPDATE_CMD_KEY_ATTRS_OFFSET 3
#define UPDATE_CMD_VAL_OFFSET 2
#define KEY_ATTRS_OPT_NAME_POS_OFFSET 1
#define KEY_ATTRS_OPT_VAL_POS_OFFSET 2

map<string, string> consoleCmd = boost::assign::map_list_of("help", "")(
    "get", " <key> [-o <long_term> <0|1>]")("aget",
                                            " <key> [-o <long_term> <0|1>]")(
    "put", " <key> <value> [-o <lock|ready|long_term> <0|1>]")(
    "aput", " <key> <value> [-o <lock|ready|long_term> <0|1>]")("status", "")(
    "remove", " <key>")("quit", "")(
    "update", " <key> [value] [-o <lock|ready|long_term> <0|1>]")(
    "aupdate", " <key> [value] [-o <lock|ready|long_term> <0|1>]")("neighbors",
                                                                   "");

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
        linenoiseAddCompletion(lc, "neighbors");
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

namespace DaqDB {

nodeCli::nodeCli(std::shared_ptr<DaqDB::KVStoreBase> &spKVStore)
    : _spKVStore(spKVStore) {
    linenoiseSetCompletionCallback(completion);
    linenoiseSetHintsCallback(hints);
}

nodeCli::~nodeCli() {}

int nodeCli::operator()() {
    auto *inLine = linenoise("daqdb> ");
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
            this->_cmdGet(strLine);
        } else if (starts_with(strLine, "p")) {
            this->_cmdPut(strLine);
        } else if (starts_with(strLine, "u")) {
            this->_cmdUpdate(strLine);
        } else if (starts_with(strLine, "ap")) {
            this->_cmdPutAsync(strLine);
        } else if (starts_with(strLine, "ag")) {
            this->_cmdGetAsync(strLine);
        } else if (starts_with(strLine, "au")) {
            this->_cmdUpdateAsync(strLine);
        } else if (starts_with(strLine, "r")) {
            this->_cmdRemove(strLine);
        } else if (starts_with(strLine, "s")) {
            this->_cmdStatus();
        } else if (starts_with(strLine, "n")) {
            this->_cmdNeighbors();
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

void nodeCli::_cmdGet(const std::string &strLine) {
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
        DaqDB::Key keyBuff;
        try {
            keyBuff = _spKVStore->AllocKey();
            /** @todo memleak - keyBuff.data */
            std::memset(keyBuff.data(), 0, keyBuff.size());
            std::memcpy(keyBuff.data(), key.c_str(), key.size());
        } catch (DaqDB::OperationFailedException &e) {
            cout << "Error: cannot allocate key buffer" << endl;
            return;
        }

        auto keyAttrs = _getKeyAttrs(GET_CMD_KEY_ATTRS_OFFSET, arguments);
        DaqDB::GetOptions options(keyAttrs, keyAttrs);

        DaqDB::Value value;
        try {
            value = _spKVStore->Get(keyBuff, std::move(options));
            string valuestr(value.data());
            cout << format("[%1%] = %2%\n") % key % valuestr;
        } catch (DaqDB::OperationFailedException &e) {
            if (e.status()() == DaqDB::KEY_NOT_FOUND) {
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

void nodeCli::_cmdGetAsync(const std::string &strLine) {
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
        DaqDB::Key keyBuff;
        try {
            keyBuff = _spKVStore->AllocKey();
            std::memset(keyBuff.data(), 0, keyBuff.size());
            std::memcpy(keyBuff.data(), key.c_str(), key.size());
        } catch (DaqDB::OperationFailedException &e) {
            cout << "Error: cannot allocate key buffer" << endl;
            return;
        }

        auto keyAttrs = _getKeyAttrs(GET_CMD_KEY_ATTRS_OFFSET, arguments);
        DaqDB::GetOptions options(keyAttrs, keyAttrs);

        try {
            _spKVStore->GetAsync(
                std::move(keyBuff),
                [&](KVStoreBase *kvs, Status status, const char *key,
                    size_t keySize, const char *value, size_t valueSize) {
                    if (!status.ok()) {
                        _statusMsgs.push_back(boost::str(
                            boost::format("Error: cannot get element: %1%") %
                            status.to_string()));
                    } else {
                        _statusMsgs.push_back(boost::str(
                            boost::format("GET[%1%]=%2% : completed") % key %
                            value));
                    }
                },
                std::move(options));
        } catch (DaqDB::OperationFailedException &e) {
            if (e.status()() == StatusCode::OFFLOAD_DISABLED_ERROR) {
                cout << format("Error: Disk offload is disabled") << endl;
            } else {
                cout << "Error: cannot get element: " << e.status().to_string()
                     << endl;
            }
        }
    }
}

DaqDB::Key nodeCli::_strToKey(const std::string &key) {
    DaqDB::Key keyBuff = _spKVStore->AllocKey();
    std::memset(keyBuff.data(), 0, keyBuff.size());
    std::memcpy(keyBuff.data(), key.c_str(), key.size());
    return keyBuff;
}

DaqDB::PrimaryKeyAttribute
nodeCli::_getKeyAttr(unsigned char start,
                     const std::vector<std::string> &cmdAttrs) {
    if ((cmdAttrs.size() < start + NUM_ELEM_KEY_ATTRS) ||
        !(starts_with(cmdAttrs[start], "-o"))) {
        return DaqDB::PrimaryKeyAttribute::EMPTY;
    }

    auto optVal = boost::lexical_cast<bool>(
        cmdAttrs[start + KEY_ATTRS_OPT_VAL_POS_OFFSET]);
    if (optVal) {
        string optName = cmdAttrs[start + KEY_ATTRS_OPT_NAME_POS_OFFSET];
        if (optName == "lock") {
            return DaqDB::PrimaryKeyAttribute::LOCKED;
        } else if (optName == "ready") {
            return DaqDB::PrimaryKeyAttribute::READY;
        } else if (optName == "long_term") {
            return DaqDB::PrimaryKeyAttribute::LONG_TERM;
        }
    }
    return DaqDB::PrimaryKeyAttribute::EMPTY;
}

DaqDB::PrimaryKeyAttribute
nodeCli::_getKeyAttrs(unsigned char start,
                      const std::vector<std::string> &cmdAttrs) {
    if (cmdAttrs.size() < start + NUM_ELEM_KEY_ATTRS) {
        return DaqDB::PrimaryKeyAttribute::EMPTY;
    }
    if ((cmdAttrs.size() - start) % NUM_ELEM_KEY_ATTRS != 0) {
        cout << "Warning: cannot parse key attributes" << endl;
        return DaqDB::PrimaryKeyAttribute::EMPTY;
    }

    unsigned char parsingPositon = start;
    DaqDB::PrimaryKeyAttribute result = DaqDB::PrimaryKeyAttribute::EMPTY;
    while (parsingPositon <= cmdAttrs.size()) {
        result = result | _getKeyAttr(parsingPositon, cmdAttrs);
        parsingPositon += NUM_ELEM_KEY_ATTRS;
    }

    return result;
}

void nodeCli::_cmdPut(const std::string &strLine) {
    vector<string> arguments;
    split(arguments, strLine, is_any_of("\t "), boost::token_compress_on);

    if (arguments.size() < 3) {
        cout << "Error: expects at least two arguments" << endl;
    } else {
        auto key = arguments[1];
        if (key.size() > _spKVStore->KeySize()) {
            cout << "Error: key size is " << _spKVStore->KeySize() << endl;
            return;
        }

        DaqDB::Key keyBuff;
        try {
            keyBuff = _strToKey(key);
        } catch (...) {
            cout << "Error: cannot allocate key buffer" << endl;
            return;
        }
        DaqDB::Value valBuff;
        try {
            valBuff = _allocValue(keyBuff, arguments[PUT_CMD_VAL_OFFSET]);
        } catch (...) {
            cout << "Error: cannot allocate value buffer" << endl;
            _spKVStore->Free(std::move(keyBuff));
            return;
        }

        auto keyAttrs = _getKeyAttrs(PUT_CMD_KEY_ATTRS_OFFSET, arguments);
        DaqDB::PutOptions options(keyAttrs);

        try {
            _spKVStore->Put(std::move(keyBuff), std::move(valBuff),
                            std::move(options));
            cout << format("Put: [%1%] = %2% (Opts:%3%)\n") % key %
                        arguments[PUT_CMD_VAL_OFFSET] % options.attr;
        } catch (DaqDB::OperationFailedException &e) {
            cout << "Error: cannot put element: " << e.status().to_string()
                 << endl;

            // @TODO jschmieg: free value
            //_spKVStore->Free(std::move(valBuff));
        }
    }
}

DaqDB::Value nodeCli::_strToValue(const std::string &valStr) {
    Value result;
    if (!starts_with(valStr, "-o")) {
        auto buffer = new char[valStr.size() + 1];
        result = Value(buffer, valStr.size() + 1);
        std::memcpy(result.data(), valStr.c_str(), valStr.size());
        result.data()[result.size() - 1] = '\0';
    }
    return result;
}

DaqDB::Value nodeCli::_allocValue(const DaqDB::Key &key,
                                  const std::string &valStr) {
    DaqDB::Value result = _spKVStore->Alloc(key, valStr.size() + 1);
    std::memcpy(result.data(), valStr.c_str(), valStr.size());
    result.data()[result.size() - 1] = '\0';
    return result;
}

void nodeCli::_cmdUpdate(const std::string &strLine) {
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
        DaqDB::Key keyBuff;
        try {
            keyBuff = _strToKey(key);
        } catch (...) {
            cout << "Error: cannot allocate key buffer" << endl;
            return;
        }

        DaqDB::Value valBuff;
        unsigned short optionStartPos = UPDATE_CMD_KEY_ATTRS_OFFSET;
        if ((arguments.size() > UPDATE_CMD_VAL_OFFSET) &&
            (!starts_with(arguments[UPDATE_CMD_VAL_OFFSET], "-o"))) {
            valBuff = _strToValue(arguments[UPDATE_CMD_VAL_OFFSET]);
        } else {
            optionStartPos = UPDATE_CMD_VAL_OFFSET;
        }

        auto keyAttrs = _getKeyAttrs(optionStartPos, arguments);
        DaqDB::UpdateOptions options(keyAttrs);

        try {
            if (valBuff.size() > 0) {
                _spKVStore->Update(std::move(keyBuff), std::move(valBuff),
                                   std::move(options));
            } else {
                _spKVStore->Update(std::move(keyBuff), std::move(options));
            }
        } catch (DaqDB::OperationFailedException &e) {
            cout << "Error: cannot update element: " << e.status().to_string()
                 << endl;
        }
    }
}

void nodeCli::_cmdUpdateAsync(const std::string &strLine) {
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
        DaqDB::Key keyBuff;
        try {
            keyBuff = _strToKey(key);
        } catch (...) {
            cout << "Error: cannot allocate key buffer" << endl;
            return;
        }

        DaqDB::Value valBuff;
        unsigned short optionStartPos = UPDATE_CMD_KEY_ATTRS_OFFSET;
        if ((arguments.size() > UPDATE_CMD_VAL_OFFSET) &&
            (!starts_with(arguments[UPDATE_CMD_VAL_OFFSET], "-o"))) {
            valBuff = _strToValue(arguments[UPDATE_CMD_VAL_OFFSET]);
        } else {
            optionStartPos = UPDATE_CMD_VAL_OFFSET;
        }

        auto keyAttrs = _getKeyAttrs(optionStartPos, arguments);
        DaqDB::UpdateOptions options(keyAttrs);

        try {
            if (valBuff.size() > 0) {
                _spKVStore->UpdateAsync(
                    std::move(keyBuff), std::move(valBuff),
                    [&](DaqDB::KVStoreBase *kvs, DaqDB::Status status,
                        const char *key, const size_t keySize,
                        const char *value, const size_t valueSize) {
                        if (!status.ok()) {
                            _statusMsgs.push_back(boost::str(
                                boost::format(
                                    "Error: cannot update element: %1%") %
                                status.to_string()));
                        } else {
                            _statusMsgs.push_back(boost::str(
                                boost::format(
                                    "UPDATE[%1%]=%2% (Opts:%3%) : completed") %
                                key % value % options.attr));
                        }
                        if (keyBuff.size() > 0)
                            _spKVStore->Free(std::move(keyBuff));
                    },
                    std::move(options));
            } else {
                _spKVStore->UpdateAsync(
                    std::move(keyBuff), std::move(options),
                    [&](DaqDB::KVStoreBase *kvs, DaqDB::Status status,
                        const char *key, const size_t keySize,
                        const char *value, const size_t valueSize) {
                        if (!status.ok()) {
                            _statusMsgs.push_back(boost::str(
                                boost::format(
                                    "Error: cannot update element: %1%") %
                                status.to_string()));
                        } else {
                            _statusMsgs.push_back(boost::str(
                                boost::format("UPDATE[%1%] (Opts:%2%) "
                                              ": completed") %
                                key % options.attr));
                        }
                        if (keyBuff.size() > 0)
                            _spKVStore->Free(std::move(keyBuff));
                    });
            }
        } catch (DaqDB::OperationFailedException &e) {
            if (e.status()() == StatusCode::OFFLOAD_DISABLED_ERROR) {
                cout << format("Error: Disk offload is disabled\n") << endl;
            } else {
                cout << "Error: cannot update element: "
                     << e.status().to_string() << endl;
            }
        }
    }
}

void nodeCli::_cmdPutAsync(const std::string &strLine) {
    vector<string> arguments;
    split(arguments, strLine, is_any_of("\t "), boost::token_compress_on);

    if (arguments.size() < 3) {
        cout << "Error: expects at least two arguments" << endl;
    } else {
        auto key = arguments[1];
        if (key.size() > _spKVStore->KeySize()) {
            cout << "Error: key size is " << _spKVStore->KeySize() << endl;
            return;
        }

        DaqDB::Key keyBuff;
        try {
            keyBuff = _strToKey(key);
        } catch (...) {
            cout << "Error: cannot allocate key buffer" << endl;
            return;
        }

        DaqDB::Value valBuff;
        try {
            valBuff = _allocValue(keyBuff, arguments[PUT_CMD_VAL_OFFSET]);
        } catch (...) {
            cout << "Error: cannot allocate value buffer" << endl;
            _spKVStore->Free(std::move(keyBuff));
            return;
        }

        auto keyAttrs = _getKeyAttrs(PUT_CMD_KEY_ATTRS_OFFSET, arguments);
        DaqDB::PutOptions options(keyAttrs);

        try {
            _spKVStore->PutAsync(
                std::move(keyBuff), std::move(valBuff),
                [&](DaqDB::KVStoreBase *kvs, DaqDB::Status status,
                    const char *key, const size_t keySize, const char *value,
                    const size_t valueSize) {
                    if (!status.ok()) {
                        _statusMsgs.push_back(boost::str(
                            boost::format("Error: cannot put element: %1%") %
                            status.to_string()));
                    } else {
                        _statusMsgs.push_back(boost::str(
                            boost::format(
                                "PUT[%1%]=%2% (Opts:%3%) : completed") %
                            key % value % options.attr));
                    }
                    if (keyBuff.size() > 0)
                        _spKVStore->Free(std::move(keyBuff));
                },
                std::move(options));
        } catch (DaqDB::OperationFailedException &e) {
            cout << "Error: cannot put element: " << e.status().to_string()
                 << endl;

            // @TODO jschmieg: free value
            // _spKVStore->Free(std::move(valBuff));
        }
    }
}

void nodeCli::_cmdRemove(const std::string &strLine) {
    vector<string> arguments;
    split(arguments, strLine, is_any_of("\t "), boost::token_compress_on);

    if (arguments.size() != 2) {
        cout << "Error: expects one argument" << endl;
    } else {

        auto key = arguments[1];
        DaqDB::Key keyBuff;
        try {
            keyBuff = _strToKey(key);
        } catch (...) {
            cout << "Error: cannot allocate key buffer" << endl;
            return;
        }

        try {
            _spKVStore->Remove(keyBuff);
            cout << format("Remove: [%1%]\n") % key;
        } catch (DaqDB::OperationFailedException &e) {
            if (e.status()() == DaqDB::KEY_NOT_FOUND) {
                cout << format("[%1%] not found\n") % key;
            } else {
                cout << "Error: cannot remove element" << endl;
            }
        }
    }
}

void nodeCli::_cmdStatus() {
    chrono::time_point<chrono::system_clock> timestamp;
    auto currentTime = chrono::system_clock::to_time_t(timestamp);
    cout << format("DHT ID = %1%\nDHT ip:port = localhost:%2%\n") %
                _spKVStore->getProperty("daqdb.dht.id") %
                _spKVStore->getProperty("daqdb.dht.port")
         << flush;
    cout << format("%1%") % _spKVStore->getProperty("daqdb.dht.status")
         << flush;
    if (_statusMsgs.size() > 0) {
        cout << endl << "Async operations:" << endl;
        for (std::string message : _statusMsgs) {
            cout << "\t- " << message << endl;
        }
        _statusMsgs.clear();
    }
}

void nodeCli::_cmdNeighbors() {
    cout << format("Neighbors:\n%1%") %
                _spKVStore->getProperty("daqdb.dht.neighbours")
         << endl;
}

} // namespace DaqDB
