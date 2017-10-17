/*
 * KVStore.cpp
 *
 *  Created on: Oct 17, 2017
 *      Author: jradtke
 */

#include "KVStore.h"

namespace DragonStore
{

KVStore::KVStore()
{
}

KVStore::~KVStore()
{
}

KVStatus
KVStore::Get(int32_t limit, int32_t keybytes, int32_t *valuebytes,
	     const char *key, char *value)
{
	return KVStatus::FAILED;
}

KVStatus
KVStore::Get(const string &key, string *valuestr)
{
	return KVStatus::FAILED;
}

KVStatus
KVStore::Put(const string &key, const string &valuestr)
{
	return KVStatus::FAILED;
}

KVStatus
KVStore::Remove(const string &key)
{
	return KVStatus::FAILED;
}

} /* namespace DragonStore */
