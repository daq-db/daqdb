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

namespace FogKV {

KVStoreBase *KVStoreBase::Open(const Options &options, Status *status)
{
	return KVStoreBaseImpl::Open(options, status);
}

KVStoreBase *KVStoreBaseImpl::Open(const Options &options, Status *status)
{
	KVStoreBaseImpl *kvs = new KVStoreBaseImpl(options);

	*status = kvs->init();
	if (status->ok())
		return dynamic_cast<KVStoreBase *>(kvs);

	delete kvs;

	return nullptr;
}

KVStoreBaseImpl::KVStoreBaseImpl(const Options &options):
	mOptions(options), mKeySize(0)
{
	if (mOptions.Runtime.io_service() == nullptr)
		m_io_service = new asio::io_service();

	for (size_t i = 0; i < options.Key.nfields(); i++)
		mKeySize += options.Key.field(i).Size;

	auto dhtPort = getOptions().Dht.Port ? : FogKV::utils::getFreePort(io_service(), 0);
	auto port = getOptions().Port ? : FogKV::utils::getFreePort(io_service(), 0);

	mDhtNode = make_unique<FogKV::CChordAdapter>(io_service(),
			dhtPort, port,
			getOptions().Dht.Id);
}

KVStoreBaseImpl::~KVStoreBaseImpl()
{
	mDhtNode.reset();

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

const Options & KVStoreBaseImpl::getOptions()
{
	return mOptions;
}

Status KVStoreBaseImpl::Put(const char *key, const char *value, size_t size)
{
	throw FUNC_NOT_IMPLEMENTED;
}

Status KVStoreBaseImpl::Put(const char *key, KVBuff *buff)
{
	throw FUNC_NOT_IMPLEMENTED;
}

Status KVStoreBaseImpl::PutAsync(const char *key, const char *value, size_t size, KVStoreBaseCallback cb)
{
	throw FUNC_NOT_IMPLEMENTED;
}

Status KVStoreBaseImpl::PutAsync(const char *key, KVBuff *buff, KVStoreBaseCallback cb)
{
	throw FUNC_NOT_IMPLEMENTED;
}

Status KVStoreBaseImpl::Get(const char *key, char *value, size_t *size)
{
	throw FUNC_NOT_IMPLEMENTED;
}

Status KVStoreBaseImpl::Get(const char *key, std::vector<char> &value)
{
	throw FUNC_NOT_IMPLEMENTED;
}

Status KVStoreBaseImpl::Get(const char *key, KVBuff *buff)
{
	throw FUNC_NOT_IMPLEMENTED;
}

Status KVStoreBaseImpl::Get(const char *key, KVBuff **buff)
{
	throw FUNC_NOT_IMPLEMENTED;
}

Status KVStoreBaseImpl::GetAsync(const char *key, KVStoreBaseCallback cb)
{
	throw FUNC_NOT_IMPLEMENTED;
}

Status KVStoreBaseImpl::GetRange(const char *beg, const char *end, std::vector<KVPair> &result)
{
	throw FUNC_NOT_IMPLEMENTED;
}

Status KVStoreBaseImpl::GetRangeAsync(const char *beg, const char *end, KVStoreBaseRangeCallback cb)
{
	throw FUNC_NOT_IMPLEMENTED;
}

KVBuff *KVStoreBaseImpl::Alloc(size_t size, const AllocOptions &options)
{
	throw FUNC_NOT_IMPLEMENTED;
}

void KVStoreBaseImpl::Free(KVBuff *buff)
{
	throw FUNC_NOT_IMPLEMENTED;
}

KVBuff *KVStoreBaseImpl::Realloc(KVBuff *buff, size_t size, const AllocOptions &options)
{
	throw FUNC_NOT_IMPLEMENTED;
}

Status KVStoreBaseImpl::Remove(const char *key)
{
	throw FUNC_NOT_IMPLEMENTED;
}

Status KVStoreBaseImpl::RemoveRange(const char *beg, const char *end)
{
	throw FUNC_NOT_IMPLEMENTED;
}

Status KVStoreBaseImpl::init()
{

	return Ok;
}

asio::io_service &KVStoreBaseImpl::io_service()
{
	if (mOptions.Runtime.io_service())
		return *mOptions.Runtime.io_service();
	return *m_io_service;
}
} // namespace FogKV
