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

#pragma once

#include <string>
#include "daqdb/Status.h"
#include "daqdb/Options.h"
#include "daqdb/KVStoreBase.h"

#include <boost/hana.hpp>

namespace DaqDB {

template <class T>
bool Set(KeyDescriptor &key)
{
	size_t sum = 0;
	size_t idx = 0;
	bool compat = true;
	T t;
	boost::hana::for_each(t, [&](auto pair) {
		size_t s = sizeof(boost::hana::second(pair));
		sum += s;
		key.field(idx, s);
	});

	// The struct has padding
	if (sizeof(T) != sum) {
		key.clear();
		return false;
	}

	return true;
}

template <class T>
class KVStore {
protected:
	KVStoreBase *base;

public:
	static KVStore<T> *Open(const Options &options, Status *status)
	{
		KVStore<T> *kv = new KVStore<T>();

		kv->base = KVStoreBase::Open(options, status);
		if (!kv->base || !kv->VerifyType()) {
			delete kv;
			return NULL;
		}

		return kv;
	}
public:
	template <class Q = T>
	typename std::enable_if<std::is_pod<Q>::value && boost::hana::Foldable<Q>::value, bool>::type VerifyType()
	{
		// This is what we can do without reflection
		if (sizeof(Q) != base->KeySize())
			return false;

		size_t sum = 0;
		int idx = 0;
		bool compat = true;
		Q t;
		boost::hana::for_each(t, [&](auto pair) {
			size_t s = sizeof(boost::hana::second(pair));
			if (s != base->getOptions().Key.field(idx++).Size)
				compat = false;
			sum += s;
		});

		// The user-defined struct has different number of fields
		if (idx != base->getOptions().Key.nfields())
			return false;

		// The user-defined struct has padding
		if (sum != sizeof(Q))
			return false;

		// The user-defined struct has different layout
		return compat;
	}

	template <class Q = T>
	typename std::enable_if<std::is_pod<Q>::value && !boost::hana::Foldable<Q>::value, bool>::type VerifyType()
	{
		// This is what we can do without reflection
		if (sizeof(Q) != base->KeySize())
			return false;

		return true;
	}

	virtual Status Put(const T &key, const char *value, size_t size) {
		return base->Put(reinterpret_cast<const char*>(&key), value, size);
	}

	virtual Status Put(const T &key, KVBuff *buff) {
		return base->Put(reinterpret_cast<const char *>(&key), buff);
	}

	virtual Status Get(const T &key, std::vector<char> &value) {
		return base->Get(reinterpret_cast<const char*>(&key), value);
	}

	virtual Status Get(const T &key, KVBuff **buff) {
		return base->Get(reinterpret_cast<const char*>(&key), buff);
	}

	virtual KVBuff *Alloc(size_t size, const AllocOptions &options = AllocOptions())
	{
		return base->Alloc(size, options);
	}

	virtual void Free(KVBuff *buff)
	{
		return base->Free(buff);
	}

	virtual KVBuff *Realloc(KVBuff *buff, size_t size, const AllocOptions &options = AllocOptions())
	{
		return base->Realloc(buff, size, options);
	}

};

}
