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

#include <chrono>
#include <boost/algorithm/string.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <iostream>
#include <map>

using namespace std;
using namespace boost::algorithm;
using boost::format;

map<string, string> consoleCmd = boost::assign::map_list_of("help", "")(
	"get", " <key>")("put", " <key> <value>")("status", "")(
	"remove", " <key>")("quit", "")("node", " <id>");

/*!
 * Provides completion functionality to dragon shell.
 *
 * @param buf	completion prefix
 * @param lc	callback functions manager
 */
void
completion(const char *buf, linenoiseCompletions *lc)
{
	if (buf[0] == 'h') {
		linenoiseAddCompletion(lc, "help");
	} else if (buf[0] == 'q') {
		linenoiseAddCompletion(lc, "quit");
	} else if (buf[0] == 'g') {
		linenoiseAddCompletion(lc, "get");
	} else if (buf[0] == 'p') {
		linenoiseAddCompletion(lc, "put");
	} else if (buf[0] == 's') {
		linenoiseAddCompletion(lc, "status");
	} else if (buf[0] == 'n') {
		linenoiseAddCompletion(lc, "node");
	} else if (buf[0] == 'r') {
		linenoiseAddCompletion(lc, "remove");
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
char *
hints(const char *buf, int *color, int *bold)
{
	*color = consoleHintColor;
	char *result = nullptr;

	if (consoleCmd.count(buf)) {
		result = const_cast<char *>(consoleCmd[buf].c_str());
	}

	return result;
}

namespace FogKV
{

nodeCli::nodeCli(std::shared_ptr<FogKV::KVStoreBase> &spKVStore) : _spKVStore(spKVStore)
{
	linenoiseSetCompletionCallback(completion);
	linenoiseSetHintsCallback(hints);
}

nodeCli::~nodeCli()
{
}

int
nodeCli::operator()()
{
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

	if (isEmpty || starts_with(strLine, "help")) {
		cout << "Following commands supported:" << endl;
		for (auto cmdEntry : consoleCmd) {
			cout << "\t- " << cmdEntry.first << cmdEntry.second
			     << endl;
		}
	} else if (starts_with(strLine, "g")) {
		this->cmdGet(strLine);
	} else if (starts_with(strLine, "p")) {
		this->cmdPut(strLine);
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

	return true;
}

void
nodeCli::cmdGet(std::string &strLine)
{
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

		vector<char> value;
		auto actionStatus = _spKVStore->Get(key.c_str(), value);
		if (actionStatus.ok()) {
			string valuestr(value.begin(), value.end());
			cout << format("[%1%] = %2%\n") % key % valuestr;
		} else if (actionStatus() == FogKV::KeyNotFound) {
			cout << format("[%1%] not found\n") % key;
		} else {
			cout << "Error: cannot get element" << endl;
		}
	}
}

void
nodeCli::cmdPut(std::string &strLine)
{
	vector<string> arguments;
	split(arguments, strLine, is_any_of("\t "), boost::token_compress_on);

	if (arguments.size() != 3) {
		cout << "Error: expects two arguments" << endl;
	} else {
		auto key = arguments[1];
		if (key.size() > _spKVStore->KeySize()) {
			cout << "Error: kay size is " << _spKVStore->KeySize() << endl;
			return;
		}

		auto value = arguments[2];
		auto actionStatus = _spKVStore->Put(key.c_str(), value.c_str(), value.size());
		if (actionStatus.ok()) {
			cout << format("Put: [%1%] = %2%\n") % key % value;
		} else {
			cout << "Error: cannot put element" << endl;
		}
	}
}

void
nodeCli::cmdRemove(std::string &strLine)
{
	vector<string> arguments;
	split(arguments, strLine, is_any_of("\t "), boost::token_compress_on);

	if (arguments.size() != 2) {
		cout << "Error: expects one argument" << endl;
	} else {
		auto key = arguments[1];
		auto actionStatus = _spKVStore->Remove(key.c_str());
		if (actionStatus.ok()) {
			cout << format("Remove: [%1%]\n") % key;
		} else if (actionStatus() == FogKV::KeyNotFound) {
			cout << format("[%1%] not found\n") % key;
		} else {
			cout << "Error: cannot remove element" << endl;
		}
	}
}

Json::Value nodeCli::getPeersJson()
{
	Json::Reader reader;
	Json::Value peers;
	std::string peersStr = _spKVStore->getProperty("fogkv.dht.peers");
	reader.parse(peersStr, peers);

	return peers;
}

void
nodeCli::cmdStatus()
{
	Json::Value peers = getPeersJson();

	auto npeers = peers.size();
	chrono::time_point<chrono::system_clock> timestamp;
	auto currentTime = chrono::system_clock::to_time_t(timestamp);

	if (!npeers) {
		cout << format("%1%\tno DHT peer(s)\n") %
					std::ctime(&currentTime);
			return;
	}

	cout << format("%1%\t%2% DHT "
			 "peers found") %
			std::ctime(&currentTime) % npeers << endl;

	for (int i = 0; i < npeers; i++) {
		cout << "[" << i << "]: " << peers[i].toStyledString();
	}


}

void
nodeCli::cmdNodeStatus(std::string &strLine)
{
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
					_spKVStore->getProperty("fogkv.listener.dht_port") << endl;
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
