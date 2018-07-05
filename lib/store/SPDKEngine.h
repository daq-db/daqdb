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

#ifndef LIB_STORE_RTREE_H_
#define LIB_STORE_RTREE_H_
#include <climits>
#include <cmath>
#include <iostream>
#include "spdk/bdev.h"
#include "spdk/io_channel.h"
#include "spdk/queue.h"
#include "spdk/env.h"
#include "FogKV/Status.h"

#include "OffloadEngine.h"

namespace FogKV {

class SPDKEngine : public FogKV::OffloadEngine {
  public:
    SPDKEngine();
    ~SPDKEngine();
    string Engine() final { return "SPDKEngine"; }

    StatusCode Get(const char *key, char **value, size_t *size) final;
    StatusCode Store(const char *key, char *value) final;
    StatusCode Remove(const char *key) final;

  private:
	struct spdk_bdev *bdev;
	struct spdk_bdev_desc *desc;
	struct spdk_io_channel *ch;

	static void msg_fn(spdk_thread_fn fn, void *ctx, void *thread_ctx);
	static void bdev_io_cb(spdk_bdev_io *bdev_io, bool success, void *cb_arg);

};
} // namespace FogKV
#endif /* LIB_STORE_RTREE_H_ */
