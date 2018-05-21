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

#pragma once

#include <iostream>
#include <linenoise.h>
#include <FogKV/KVStoreBase.h>
#include <json/json.h>
#include <memory>

namespace
{
const unsigned int consoleHintColor = 35; // dark red
};

namespace FogKV
{

/*!
 * Dragon shell interpreter.
 * Created for test purposes - to allow performing quick testing of the node.
 */
class nodeCli {
public:
	nodeCli(std::shared_ptr<FogKV::KVStoreBase> &spDragonSrv);
	virtual ~nodeCli();

	/*!
	 * Waiting for user input, executes defined commands
	 *
	 * @return false if user choose "quit" command, otherwise true
	 */
	int operator()();

private:
	void cmdGet(const std::string &strLine);
	void cmdPut(const std::string &strLine);
	void cmdRemove(const std::string &strLine);
	void cmdStatus();
	void cmdNodeStatus(const std::string &strLine);

	std::unique_ptr<FogKV::Key> strToKey(const std::string &key);
	FogKV::Value strToValue(const std::string &val);

	std::shared_ptr<FogKV::KVStoreBase> _spKVStore;

	Json::Value getPeersJson();
};

}
