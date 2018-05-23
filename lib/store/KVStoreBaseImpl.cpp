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

#include "KVStoreBaseImpl.h"
#include <FogKV/Types.h>
#include <json/json.h>
#include "common.h"
#include "../dht/DhtUtils.h"
#include <future>

namespace FogKV {

KVStoreBase *KVStoreBase::Open(const Options &options)
{
	return KVStoreBaseImpl::Open(options);
}

KVStoreBase *KVStoreBaseImpl::Open(const Options &options)
{
	KVStoreBaseImpl *kvs = new KVStoreBaseImpl(options);

	kvs->init();

	return kvs;
}

size_t KVStoreBaseImpl::KeySize()
{
	return mKeySize;
}

const Options & KVStoreBaseImpl::getOptions()
{
	return mOptions;
}

void KVStoreBaseImpl::Put(Key &&key, Value &&val, const PutOptions &options)
{
	std::unique_lock<std::mutex> l(mLock);

	std::string keyStr(key.data(), mKeySize);
	std::string valStr(val.data(), val.size());
	KVStatus s = mPmemkv->Put(keyStr, valStr);
	Free(std::move(key));
	Free(std::move(val));
	if (s != OK)
		throw OperationFailedException(EINVAL);

	/** @TODO jradtke: initial implementation, msg format and cpl function needed */
	if (mRqstPooler) {
		mRqstPooler->EnqueueMsg(new RqstMsg());
	}
}

void KVStoreBaseImpl::PutAsync(Key &&key, Value &&value, KVStoreBasePutCallback cb, const PutOptions &options)
{
	std::async(std::launch::async, [&] {
		try {
			Put(std::move(key), std::move(value));
			cb(this, Ok, key, value);
		} catch (OperationFailedException e) {
			cb(this, e.status(), key, value);
		}
	});
}

Value KVStoreBaseImpl::Get(const Key &key, const GetOptions &options)
{
	std::unique_lock<std::mutex> l(mLock);

	std::string keyStr(key.data(), mKeySize);
	std::string valStr;
	KVStatus s = mPmemkv->Get(keyStr, &valStr);
	if (s != OK) {
		if (s == NOT_FOUND)
			throw OperationFailedException(KeyNotFound);

		throw OperationFailedException(EINVAL);
	}

	Value value(new char [valStr.length() + 1], valStr.length() + 1);
	std::memcpy(value.data(), valStr.c_str(), value.size());
	value.data()[value.size() - 1] = '\0';
	return value;
}

void KVStoreBaseImpl::GetAsync(const Key &key, KVStoreBaseGetCallback cb, const GetOptions &options)
{
	std::async(std::launch::async, [&] {
		try {
			Value val = Get(key);
			cb(this, Ok, key, val);
		} catch (OperationFailedException e) {
			Value val;
			cb(this, e.status(), key, val);
		}
	});
}

Key KVStoreBaseImpl::GetAny(const GetOptions &options)
{
	throw FUNC_NOT_IMPLEMENTED;
}

void KVStoreBaseImpl::GetAnyAsync(KVStoreBaseGetAnyCallback cb, const GetOptions &options)
{
	throw FUNC_NOT_IMPLEMENTED;
}

void KVStoreBaseImpl::Update(const Key &key, Value &&val, const UpdateOptions &options)
{
	std::unique_lock<std::mutex> l(mLock);

	std::string keyStr(key.data(), mKeySize);
	std::string valStr(val.data(), val.size());
	KVStatus s = mPmemkv->Put(keyStr, valStr);
	Free(std::move(val));

	if (s != OK)
		throw OperationFailedException(EINVAL);
}

void KVStoreBaseImpl::Update(const Key &key, const UpdateOptions &options)
{
	throw FUNC_NOT_IMPLEMENTED;
}

void KVStoreBaseImpl::UpdateAsync(const Key &key, Value &&value, KVStoreBaseUpdateCallback cb, const UpdateOptions &options)
{
	std::async(std::launch::async, [&] {
		try {
			Update(key, std::move(value));
			cb(this, Ok, key, value);
		} catch (OperationFailedException e) {
			cb(this, e.status(), key, value);
		}
	});
}

void KVStoreBaseImpl::UpdateAsync(const Key &key, const UpdateOptions &options, KVStoreBaseUpdateCallback cb)
{
	throw FUNC_NOT_IMPLEMENTED;
}

std::vector<KVPair> KVStoreBaseImpl::GetRange(const Key &beg, const Key &end, const GetOptions &options)
{
	throw FUNC_NOT_IMPLEMENTED;
}

void KVStoreBaseImpl::GetRangeAsync(const Key &beg, const Key &end, KVStoreBaseRangeCallback cb, const GetOptions &options)
{
	throw FUNC_NOT_IMPLEMENTED;
}

void KVStoreBaseImpl::Remove(const Key &key)
{
	std::unique_lock<std::mutex> l(mLock);

	std::string keyStr(key.data(), mKeySize);
	KVStatus s = mPmemkv->Remove(keyStr);
	if (s != OK) {
		if (s == NOT_FOUND)
			throw OperationFailedException(KeyNotFound);

		throw OperationFailedException(EINVAL);
	}
}

void KVStoreBaseImpl::RemoveRange(const Key &beg, const Key &end)
{
	throw FUNC_NOT_IMPLEMENTED;
}

Value KVStoreBaseImpl::Alloc(size_t size, const AllocOptions &options)
{
	return Value(new char[size], size);
}

void KVStoreBaseImpl::Free(Value &&value)
{
	delete [] value.data();
}

void KVStoreBaseImpl::Realloc(Value &value, size_t size, const AllocOptions &options)
{
	throw FUNC_NOT_IMPLEMENTED;
}

void KVStoreBaseImpl::ChangeOptions(Value &value, const AllocOptions &options)
{
	throw FUNC_NOT_IMPLEMENTED;
}

Key KVStoreBaseImpl::AllocKey(const AllocOptions &options)
{
	return Key(new char[mKeySize], mKeySize);
}

void KVStoreBaseImpl::Free(Key &&key)
{
	delete [] key.data();
}

void KVStoreBaseImpl::ChangeOptions(Key &key, const AllocOptions &options)
{
	throw FUNC_NOT_IMPLEMENTED;
}

KVStoreBaseImpl::KVStoreBaseImpl(const Options &options):
	mOptions(options), mKeySize(0)
{
	if (mOptions.Runtime.io_service() == nullptr)
		m_io_service = new asio::io_service();

	for (size_t i = 0; i < options.Key.nfields(); i++)
		mKeySize += options.Key.field(i).Size;
}

KVStoreBaseImpl::~KVStoreBaseImpl()
{
	mDhtNode.reset();

	pmemkv::KVEngine::Close(mPmemkv.get());
	mPmemkv.reset();

	if (m_io_service)
		delete m_io_service;
}

std::string KVStoreBaseImpl::getProperty(const std::string &name)
{
	if (name == "fogkv.dht.id")
		return std::to_string(mDhtNode->getDhtId());
	if (name == "fogkv.listener.ip")
		return mDhtNode->getIp();
	if (name == "fogkv.listener.port")
		return std::to_string(mDhtNode->getPort());
	if (name == "fogkv.listener.dht_port")
		return std::to_string(mDhtNode->getDragonPort());
	if (name == "fogkv.pmem.path")
		return mOptions.PMEM.Path;
	if (name == "fogkv.pmem.size")
		return std::to_string(mOptions.PMEM.Size);
	if (name == "fogkv.KVEngine")
		return mPmemkv->Engine();
	if (name == "fogkv.dht.peers") {
		Json::Value peers;
		std::vector<FogKV::PureNode*> peerNodes;
		mDhtNode->getPeerList(peerNodes);

		int i = 0;
		for (auto peer: peerNodes) {
			peers[i]["id"] = std::to_string(peer->getDhtId());
			peers[i]["port"] = std::to_string(peer->getPort());
			peers[i]["ip"] = peer->getIp();
			peers[i]["dht_port"] = std::to_string(peer->getDragonPort());
		}

		Json::FastWriter writer;
		return writer.write(peers);
	}
	return "";
}

void KVStoreBaseImpl::init()
{
	auto dhtPort = getOptions().Dht.Port ? : FogKV::utils::getFreePort(io_service(), 0);
	auto port = getOptions().Port ? : FogKV::utils::getFreePort(io_service(), 0);

	mDhtNode.reset(new FogKV::CChordAdapter(io_service(),
			dhtPort, port,
			getOptions().Dht.Id));

	mPmemkv.reset(pmemkv::KVEngine::Open(
			mOptions.KVEngine, mOptions.PMEM.Path, mOptions.PMEM.Size));

	/**
	 * @TODO jradtke: initial implementation, will be moved to better place.
	 * 				  SPDK init messages should be hidden.
	 */
	struct spdk_env_opts opts;
	spdk_env_opts_init(&opts);
	opts.name = "FogKV";
	opts.shm_id = 0;
	if (spdk_env_init(&opts) == 0) {
		mRqstPooler.reset(new FogKV::RqstPooler());
	}

	if (mPmemkv == nullptr)
		throw OperationFailedException(errno, ::pmemobj_errormsg());
}

asio::io_service &KVStoreBaseImpl::io_service()
{
	if (mOptions.Runtime.io_service())
		return *mOptions.Runtime.io_service();
	return *m_io_service;
}

} // namespace FogKV
