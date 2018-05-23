/**
 * Copyright 2018, Intel Corporation
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

#include "MinidaqAsyncReadoutNode.h"

using namespace std;

namespace FogKV {

MinidaqAsyncReadoutNode::MinidaqAsyncReadoutNode(KVStoreBase *kvs) :
	MinidaqReadoutNode(kvs)
{
}

MinidaqAsyncReadoutNode::~MinidaqAsyncReadoutNode()
{
}

void MinidaqAsyncReadoutNode::Task(int executorId, std::atomic<std::uint64_t> &cnt,
								   std::atomic<std::uint64_t> &cntErr)
{
	Key key = kvs->AllocKey();
	MinidaqKey *keyp = reinterpret_cast<MinidaqKey *>(key.data());
	keyp->subdetector_id = id;
	keyp->run_id = runId;
	keyp->event_id = currEventId[executorId];
	currEventId[executorId] += nTh;

	FogKV::Value value = kvs->Alloc(1024);

	try {
		kvs->PutAsync(std::move(key), std::move(value),
					  [&] (FogKV::KVStoreBase *kvs, FogKV::Status status,
						   const FogKV::Key &key, const FogKV::Value &val) {
						if (!status.ok()) {
							cntErr++;
							return;
						}
						cnt++;
						});
	} catch (...) {
		cntErr++;
	}
}

}
