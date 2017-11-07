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

#include "SimFogKV.h"

#include <log4cxx/basicconfigurator.h>
#include <log4cxx/consoleappender.h>
#include <log4cxx/helpers/exception.h>
#include <log4cxx/logger.h>
#include <log4cxx/simplelayout.h>
#include <log4cxx/xml/domconfigurator.h>

#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>

using namespace log4cxx;
using namespace log4cxx::xml;
using namespace log4cxx::helpers;

namespace Dragon {

SimFogKV::SimFogKV(const std::string &diskPath, const unsigned int elementSize,
		const unsigned int limitGet, const unsigned int limitPut) :
		_diskWorker(diskPath, elementSize), _limit_get(limitGet), _limit_put(
				limitPut) {
}

SimFogKV::~SimFogKV() {
}

KVStatus SimFogKV::Put(const std::string& key, const std::vector<char> &value) {

	KVStatus status;
	unsigned long int startLba;

	if ((_limit_put > 0) && (_aepWorker.getWriteCounter() > _limit_put)) {
		return KVStatus::FAILED;
	}

	std::tie(status, startLba) = _diskWorker.Add(value);

	auto actionStatus = _aepWorker.Put(key, std::to_string(startLba));

	return KVStatus::OK;
}

KVStatus SimFogKV::Get(const std::string& key, std::vector<char> &value) {

	if ((_limit_get > 0) && (_aepWorker.getReadCounter() > _limit_get)) {
		return KVStatus::FAILED;
	}

	string valuestr;
	auto actionStatus = _aepWorker.Get(key, &valuestr);

	auto status = _diskWorker.Read(
			boost::lexical_cast<unsigned long int>(valuestr), value);

	return KVStatus::OK;
}

std::tuple<float, float> SimFogKV::getIoStat() {

	return _aepWorker.getIoStat();
}

void SimFogKV::setIOLimit(const unsigned int limitGet,
		const unsigned int limitPut) {
	_limit_get = limitGet;
	_limit_put = limitPut;
}

} /* namespace Dragon */

