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

using namespace std;

#include <iostream>

#include "PmemKVStore.h"

namespace bf = boost::filesystem;

namespace
{
const string pmemKvEngine = "kvtree";
const string pmemKvBasePath = "/dev/shm/fogkv"; //!< @todo jradtke This should be configurable
};

namespace FogKV
{

PmemKVStore::PmemKVStore(int nodeId, const bool temporaryDb)
    : _kvStoreFile(pmemKvBasePath), _temporaryDb(temporaryDb)
{
	_kvStoreFile += to_string(nodeId);

	this->_spStore.reset(pmemkv::KVEngine::Open(
		pmemKvEngine, _kvStoreFile.string(), PMEMOBJ_MIN_POOL));
}

PmemKVStore::~PmemKVStore()
{
	try {
		if (isTemporaryDb()) {
			bf::remove(_kvStoreFile);
		}
	} catch (bf::filesystem_error &e) {
		//! @todo jradtke Do we need logger?
	}
}

KVStatus
PmemKVStore::Get(int32_t limit, int32_t keybytes, int32_t *valuebytes,
		 const char *key, char *value)
{
	return this->_spStore->Get(limit, keybytes, valuebytes, key, value);
}

KVStatus
PmemKVStore::Get(const string &key, string *valuestr)
{
	return this->_spStore->Get(key, valuestr);
}

KVStatus
PmemKVStore::Put(const string &key, const string &valuestr)
{
	return this->_spStore->Put(key, valuestr);
}

KVStatus
PmemKVStore::Remove(const string &key)
{
	return this->_spStore->Remove(key);
}

} /* namespace DragonStore */
