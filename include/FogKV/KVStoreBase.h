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
#include <functional>
#include <boost/hana.hpp>

namespace FogKV {

class KVStoreBase {
public:
	using KVStoreBaseCallback = std::function<void (KVStoreBase *kvs, Status status, const char *key, const KVBuff &buff)>;
	using KVStoreBaseRangeCallback = std::function<void (KVStoreBase *kvs, Status status, std::vector<KVPair> &results)>;

	static KVStoreBase *Open(const Options &options, Status *status);
public:
	virtual size_t KeySize() = 0;
	virtual const Options &getOptions() = 0;
	virtual std::string getProperty(const std::string &name) = 0;

	virtual Status Put(const char *key, const char *value, size_t size) = 0;
	virtual Status Put(const char *key, KVBuff *buff) = 0;

	virtual Status PutAsync(const char *key, const char *value, size_t size, KVStoreBaseCallback cb) = 0;
	virtual Status PutAsync(const char *key, KVBuff *buff, KVStoreBaseCallback cb) = 0;

	virtual Status Get(const char *key, std::vector<char> &value) = 0;
	virtual Status Get(const char *key, KVBuff **buff) = 0;
	virtual Status Get(const char *key, KVBuff *buff) = 0;
	virtual Status Get(const char *key, char *value, size_t *size) = 0;

	virtual Status GetAsync(const char *key, KVStoreBaseCallback cb) = 0;

	virtual Status GetRange(const char *beg, const char *end, std::vector<KVPair> &result) = 0;
	virtual Status GetRangeAsync(const char *beg, const char *end, KVStoreBaseRangeCallback cb) = 0;

	virtual Status Remove(const char *key) = 0;
	virtual Status RemoveRange(const char *beg, const char *end) = 0;

	virtual KVBuff *Alloc(size_t size, const AllocOptions &options = AllocOptions()) = 0;
	virtual void Free(KVBuff *buff) = 0;
	virtual KVBuff *Realloc(KVBuff *buff, size_t size, const AllocOptions &options = AllocOptions()) = 0;
};

} // namespace FogKV
