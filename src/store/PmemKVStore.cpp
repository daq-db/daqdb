/*
 * PmemKVStore.cpp
 *
 *  Created on: Oct 17, 2017
 *      Author: jradtke
 */

using namespace std;

#include <iostream>

#include "PmemKVStore.h"

namespace bf = boost::filesystem;

namespace
{
const string pmemKvEngine = "kvtree";
const string pmemKvBasePath = "/dev/shm/fogkv";
};

namespace DragonStore
{

PmemKVStore::PmemKVStore(int nodeId) : kvStoreFile(pmemKvBasePath)
{
	kvStoreFile += to_string(nodeId);

	this->_spStore.reset(pmemkv::KVEngine::Open(
		pmemKvEngine, kvStoreFile.string(), PMEMOBJ_MIN_POOL));
}

PmemKVStore::~PmemKVStore()
{
	try {
		bf::remove(kvStoreFile);
	} catch (bf::filesystem_error &e) {
		// @TODO jradtke: Do we need logger?
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
