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

#include <mutex>
#include "RqstPooler.h"
#include <FogKV/KVStoreBase.h>
#include <dht/CChordNode.h>
#include <dht/DhtNode.h>
#include <pmemkv.h>

namespace FogKV {

class KVStoreBaseImpl : public KVStoreBase {
public:
	static KVStoreBase *Open(const Options &options);
public:
	virtual size_t KeySize();
	virtual const Options &getOptions();
	virtual std::string getProperty(const std::string &name);
	virtual void Put(Key &&key, Value &&value, const PutOptions &options = PutOptions());
	virtual void PutAsync(Key &&key, Value &&value, KVStoreBasePutCallback cb, const PutOptions &options = PutOptions());
	virtual Value Get(const Key &key, const GetOptions &options = GetOptions());
	virtual void GetAsync(const Key &key, KVStoreBaseGetCallback cb, const GetOptions &options = GetOptions());
	virtual void Update(const Key &key, Value &&value, const UpdateOptions &options = UpdateOptions());
	virtual void Update(const Key &key, const UpdateOptions &options);
	virtual void UpdateAsync(const Key &key, Value &&value, KVStoreBaseUpdateCallback cb, const UpdateOptions &options = UpdateOptions());
	virtual void UpdateAsync(const Key &key, const UpdateOptions &options, KVStoreBaseUpdateCallback cb);
	virtual std::vector<KVPair> GetRange(const Key &beg, const Key &end, const GetOptions &options = GetOptions());
	virtual void GetRangeAsync(const Key &beg, const Key &end, KVStoreBaseRangeCallback cb, const GetOptions &options = GetOptions());
	virtual Key GetAny(const GetOptions &options = GetOptions());
	virtual void GetAnyAsync(KVStoreBaseGetAnyCallback cb, const GetOptions &options = GetOptions());
	virtual void Remove(const Key &key);
	virtual void RemoveRange(const Key &beg, const Key &end);
	virtual Value Alloc(size_t size, const AllocOptions &options = AllocOptions());
	virtual void Free(Value &&value);
	virtual void Realloc(Value &value, size_t size, const AllocOptions &options = AllocOptions());
	virtual void ChangeOptions(Value &value, const AllocOptions &options);
	virtual Key AllocKey(const AllocOptions &options = AllocOptions());
	virtual void Free(Key &&key);
	virtual void ChangeOptions(Key &key, const AllocOptions &options);
protected:
	KVStoreBaseImpl(const Options &options);
	virtual ~KVStoreBaseImpl();

	void init();
	void registerProperties();

	asio::io_service &io_service();
	asio::io_service *m_io_service;

	size_t mKeySize;
	Options mOptions;
	std::unique_ptr<FogKV::DhtNode> mDhtNode;
	std::shared_ptr<pmemkv::KVEngine> mPmemkv;
	std::mutex mLock;

	std::vector<RqstPooler*> _rqstPoolers;
};

} //namespace FogKV

