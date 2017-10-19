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

#include "DragonCli.h"

#include <boost/algorithm/string.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/format.hpp>
#include <iostream>
#include <map>

using namespace std;
using namespace boost::algorithm;
using boost::format;

map<string, string> consoleCmd = boost::assign::map_list_of("help", "")(
	"get", " <key>")("put", " <key> <value>")("status", "")(
	"remove", " <key>")("quit", "");

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

namespace Dragon
{

DragonCli::DragonCli(std::shared_ptr<Dragon::DragonSrv> &spDragonSrv) : _spDragonSrv(spDragonSrv)
{
	linenoiseSetCompletionCallback(completion);
	linenoiseSetHintsCallback(hints);
}

DragonCli::~DragonCli()
{
}

int
DragonCli::operator()()
{
	auto *inLine = linenoise("dragon> ");
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
	} else if (starts_with(strLine, "q")) {
		return false;
	} else {
		cout << format("Unreconized command: %1%\n") % strLine;
	}

	return true;
}

void
DragonCli::cmdGet(std::string &strLine)
{
	vector<string> arguments;
	split(arguments, strLine, is_any_of("\t "), boost::token_compress_on);

	if (arguments.size() != 2) {
		cout << "Error: expects one argument" << endl;
	} else {
		auto key = arguments[1];
		string valuestr;
		auto actionStatus = _spDragonSrv->getKvStore()->Get(key, &valuestr);
		if (actionStatus == KVStatus::OK) {
			cout << format("[%1%] = %2%\n") % key % valuestr;
		} else if (actionStatus == KVStatus::NOT_FOUND) {
			cout << format("[%1%] not found\n") % key;
		} else {
			cout << "Error: cannot get element" << endl;
		}
	}
}

void
DragonCli::cmdPut(std::string &strLine)
{
	vector<string> arguments;
	split(arguments, strLine, is_any_of("\t "), boost::token_compress_on);

	if (arguments.size() != 3) {
		cout << "Error: expects two arguments" << endl;
	} else {
		auto key = arguments[1];
		auto value = arguments[2];
		auto actionStatus = _spDragonSrv->getKvStore()->Put(key, value);
		if (actionStatus == KVStatus::OK) {
			cout << format("Put: [%1%] = %2%\n") % key % value;
		} else {
			cout << "Error: cannot put element" << endl;
		}
	}
}

void
DragonCli::cmdRemove(std::string &strLine)
{
	vector<string> arguments;
	split(arguments, strLine, is_any_of("\t "), boost::token_compress_on);

	if (arguments.size() != 2) {
		cout << "Error: expects one argument" << endl;
	} else {
		auto key = arguments[1];
		auto actionStatus = _spDragonSrv->getKvStore()->Remove(key);
		if (actionStatus == KVStatus::OK) {
			cout << format("Remove: [%1%]\n") % key;
		} else if (actionStatus == KVStatus::NOT_FOUND) {
			cout << format("[%1%] not found\n") % key;
		} else {
			cout << "Error: cannot remove element" << endl;
		}
	}
}

void
DragonCli::cmdStatus()
{
	//!	@todo jradtke Add more information to status
	cout << _spDragonSrv->getDhtPeerStatus() << endl;
}

} /* namespace Dragon */
