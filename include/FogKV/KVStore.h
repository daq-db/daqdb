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

#include <string>
#include "FogKV/Status.h"
#include "FogKV/Options.h"
#include "FogKV/KVBuff.h"
#include "FogKV/KVPair.h"
#include "FogKV/KVStoreBase.h"
#include <boost/hana.hpp>

namespace FogKV {

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

} // namespace FogKV
