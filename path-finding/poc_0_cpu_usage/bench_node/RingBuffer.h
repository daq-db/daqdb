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
#include <condition_variable>
#include <functional>

class RingBuffer {
public:
	using RingBufferFlush = std::function<bool (size_t, size_t)>;
	using RingBufferRead = std::function<ssize_t (const uint8_t *, size_t len)>;
public:
	RingBuffer(size_t size, RingBufferFlush flush = RingBufferFlush());
	virtual ~RingBuffer();

	uint8_t *get();
	size_t write(const uint8_t *ptr, size_t len);
	void read(size_t len, RingBufferRead read);
	void notifyRead(size_t len);
	void notifyWrite(size_t len);
	size_t size();
	size_t occupied();
	size_t space();
protected:
	size_t occupied_unlocked();
	size_t space_unlocked();
	std::mutex mMtx;
	std::condition_variable mCond;
	uint8_t *mBuff;
	size_t mSize;
	size_t mBegin;
	size_t mEnd;
	RingBufferFlush mFlush;
};
