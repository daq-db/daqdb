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

#include "SPDKEngine.h"
#include "FogKV/Status.h"
#include <iostream>

namespace FogKV {
#define LAYOUT "rtree"

SPDKEngine::SPDKEngine() {
	spdk_allocate_thread(msg_fn, nullptr, nullptr, this,"spdk0");
	
}


void SPDKEngine::msg_fn(spdk_thread_fn fn, void *ctx, void *thread_ctx) {

	SPDKEngine *engine = (SPDKEngine*)thread_ctx;

	engine->bdev = spdk_bdev_first();
//	spdk_bdev_open(bdev, true, nullptr, nullptr, &desc);
}



SPDKEngine::~SPDKEngine() {
	//spdk_bdev_finish();
	//
	//
}


StatusCode SPDKEngine::Get(const char *key, char **value, size_t *size) {

	/* TODO: add callbacks */
	//spdk_bdev_read(bdev, ch, value, offset, num_blocks, cb, cb_arg);


}

void SPDKEngine::bdev_io_cb(spdk_bdev_io *bdev_io, bool success, void *cb_arg) {

	//rc = mRTree->UpdateValueWrapper(key.data(), iov, key.size());

}


StatusCode SPDKEngine::Store(const char *key, char *value) {

	uint64_t offset = 0; // TODO: get next free offset
	uint64_t num_blocks = 0;
	spdk_bdev_write(desc, ch, value, offset, num_blocks, bdev_io_cb, (void*)key);

}

StatusCode SPDKEngine::Remove(const char *key) {
	//spdk_bdev_unmap(bdev, ch, offset, num_block, cb, cb_arg);

}

} // namespace FogKV
