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

#include "AepWorker.h"
#ifdef FOGKV_USE_LOG4CXX
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/consoleappender.h>
#include <log4cxx/helpers/exception.h>
#include <log4cxx/logger.h>
#include <log4cxx/simplelayout.h>
#include <log4cxx/xml/domconfigurator.h>
#endif

#include "../../../lib/store/PmemKVStore.h" //!< @todo jradtke this include should be removed

#ifdef FOGKV_USE_LOG4CXX
using namespace log4cxx;
using namespace log4cxx::xml;
using namespace log4cxx::helpers;
#endif

namespace FogKV {

AepWorker::AepWorker() {
	auto isTemporaryDb = true;

#ifdef FOGKV_USE_LOG4CXX
	LOG4CXX_INFO(log4cxx::Logger::getRootLogger(), "New PmemKVStore created");
#endif
	this->_spStore.reset(
		new FogKV::PmemKVStore(1, isTemporaryDb));
}

AepWorker::~AepWorker() {
}

KVStatus AepWorker::Put(const string& key, const string& valuestr) {
	_ioMeter._io_count_write++;
	return _spStore->Put(key, valuestr);
}

KVStatus AepWorker::Get(const string& key, string* valuestr) {
	_ioMeter._io_count_read++;
	return _spStore->Get(key, valuestr);
}

std::tuple<float, float> AepWorker::getIoStat() {
	return _ioMeter.getIoStat();
}

unsigned long int AepWorker::getReadCounter() {
	return _ioMeter._io_count_read;
}

unsigned long int AepWorker::getWriteCounter() {
	return _ioMeter._io_count_write;
}

}
