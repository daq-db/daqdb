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
#include "RingBuffer.h"

#include <string.h>
#include <assert.h>

RingBuffer::RingBuffer(size_t size, RingBufferFlush flush)
{
	mBuff = new uint8_t[size];
	mSize = size;
	mBegin = 0;
	mEnd = 0;
	mFlush = flush;
}

RingBuffer::~RingBuffer()
{
	delete [] mBuff;
}

size_t RingBuffer::space()
{
	std::unique_lock<std::mutex> l(mMtx);
	return space_unlocked();
}

size_t RingBuffer::space_unlocked()
{
	return mSize - occupied_unlocked();
}


size_t RingBuffer::occupied()
{
	std::unique_lock<std::mutex> l(mMtx);
	return occupied_unlocked();
}

size_t RingBuffer::occupied_unlocked()
{
	return (mSize + mEnd - mBegin) % mSize;
}

size_t RingBuffer::write(const uint8_t *ptr, size_t len)
{
	std::unique_lock<std::mutex> l(mMtx);
	mCond.wait(l, [&] {
		assert(space_unlocked() >= len);
		return space_unlocked() >= len;
	});

	size_t flen = mSize - mEnd > len ? len: mSize - mEnd;
	size_t slen = len - flen;
	
	memcpy(&mBuff[mEnd], ptr, flen);
	mFlush(mEnd, flen);
	if (slen) {
		memcpy(&mBuff[0], ptr + flen, slen);
		mFlush(0, slen);
	}

	size_t ret = mEnd;

	mEnd = (mEnd + len) % mSize;

	l.unlock();
	mCond.notify_one();

	return ret;
}

void RingBuffer::read(size_t len, RingBufferRead read)
{
	size_t rd = 0;
	std::unique_lock<std::mutex> l(mMtx);
	mCond.wait(l, [&] {
		assert(occupied_unlocked() >= len);
		return occupied_unlocked() >= len;
	});

	while (rd < len) {
		size_t lend = mSize - mBegin;
		size_t l = len - rd;
		l = l < lend ? l : lend;

		read(&mBuff[mBegin], l);

		rd += l;

		mBegin = (mBegin + l) % mSize;
	}

	l.unlock();
	if (rd)
		mCond.notify_one();

}

void RingBuffer::notifyWrite(size_t len)
{
	std::unique_lock<std::mutex> l(mMtx);
	mCond.wait(l, [&] {
		return space_unlocked() >= len;
	});

	mEnd = (mEnd + len) % mSize;

	l.unlock();
	mCond.notify_one();
}

void RingBuffer::notifyRead(size_t len)
{
	std::unique_lock<std::mutex> l(mMtx);
	mBegin = (mBegin + len) % mSize;
	l.unlock();
	mCond.notify_one();
}

uint8_t * RingBuffer::get()
{
	return mBuff;
}

size_t RingBuffer::size()
{
	return mSize;
}
