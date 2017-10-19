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

#ifndef SRC_NODE_DRAGONCLI_H_
#define SRC_NODE_DRAGONCLI_H_

#include "DragonSrv.h"
#include <iostream>
#include <linenoise.h>

namespace
{
const unsigned int consoleHintColor = 35; // dark red
};

namespace Dragon
{

/*!
 * Dragon shell interpreter.
 * Created for test purposes - to allow performing quick testing of the node.
 */
class DragonCli {
public:
	DragonCli(std::shared_ptr<Dragon::DragonSrv> &spDragonSrv);
	virtual ~DragonCli();

	/*!
	 * Waiting for user input, executes defined commands
	 *
	 * @return false if user choose "quit" command, otherwise true
	 */
	int operator()();

private:
	void cmdGet(std::string &strLine);
	void cmdPut(std::string &strLine);
	void cmdRemove(std::string &strLine);
	void cmdStatus();

	std::shared_ptr<Dragon::DragonSrv> _spDragonSrv;
};

} /* namespace Dragon */

#endif /* SRC_NODE_DRAGONCLI_H_ */
