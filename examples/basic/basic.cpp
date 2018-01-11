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

#include "FogKV/KVStoreBase.h"

#include <assert.h>
#include <cstring>
#include <limits>

#if HAS_BOOST_HANA
#include "FogKV/KVStore.h"
#endif

struct Key {
	Key() = default;
	Key(uint16_t s, uint16_t r, uint64_t e) : subdetector_id(s), run_id(r), event_id(e) {}
	uint16_t subdetector_id;
	uint16_t run_id;
	uint64_t event_id;
};

struct Key1 {
	uint16_t subdetector_id;
	uint32_t run_id; // different size
	uint64_t event_id;
};

struct Key2 {
	// uint16_t subdetector_id;
	uint32_t run_id; // different size
	uint64_t event_id;
};

struct Key3 {
	// uint16_t subdetector_id;
	uint32_t run_id; // different size
	uint64_t event_id;
};

#if HAS_BOOST_HANA
BOOST_HANA_ADAPT_STRUCT(Key, subdetector_id, run_id, event_id);
BOOST_HANA_ADAPT_STRUCT(Key2, run_id, event_id);
#endif

int KVStoreBaseExample()
{
	FogKV::Options options;

	/*
	 * key : {
	 *	size: 16,
	 *	fields : [
	 *		{
	 *			size: 	2,
	 *		},
	 *		{
	 *			size:	2,
	 *		},
	 *		{
	 *			size:	8,
	 *		},
	 *	],
	 * },
	 */
	options.Key.field(0, sizeof(Key::subdetector_id));
	options.Key.field(1, sizeof(Key::run_id));
	options.Key.field(2, sizeof(Key::event_id));


	// Open KV store
	FogKV::Status s;
       	FogKV::KVStoreBase *kvs = FogKV::KVStoreBase::Open(options, &s);
	if (!s.ok()) {
		// error
		return 1;
	}

	// Put using (char *) for key and value
	{
		Key key(1, 1, 1);
		char *keyp = reinterpret_cast<char *>(&key);
		char data[256] = {0, };

		s = kvs->Put(keyp, data, sizeof(data));
		assert(s.ok());
	}

	// Put using user-defined key structure for key and preallocated
	// buffer. The Put with such a buffer is zero-copy operation as the
	// buffer is allocated from persistent memory.
	// The user is _not_ allowed to use the buffer after Put operation.
	{
		FogKV::KVBuff *buff = kvs->Alloc(256);

		std::memset(buff->data(), 0, buff->size());

		Key key(1, 1, 2);
		char *keyp = reinterpret_cast<char *>(&key);

		s = kvs->Put(keyp, buff);
	}

	// Asynchronous put
	{
		FogKV::KVBuff *buff = kvs->Alloc(256);

		std::memset(buff->data(), 0, buff->size());

		Key key(1, 1, 5);
		char *keyp = reinterpret_cast<char *>(&key);

		s = kvs->PutAsync(keyp, buff, [&] (FogKV::KVStoreBase *kvs, FogKV::Status status, const char *key, const FogKV::KVBuff &buff) {
			if (!status.ok())
				return;
			// Done
		});
	}

	// Get using (char *) as a key and std::vector as a value.
	{
		Key key(1, 1, 3);
		char *keyp = reinterpret_cast<char *>(&key);
		std::vector<char> value;

		s = kvs->Get(keyp, value);
	}

	// Get using (char *) as a key and KVBuff
	{
		Key key(1, 1, 3);
		char *keyp = reinterpret_cast<char *>(&key);
		FogKV::KVBuff *value;

		s = kvs->Get(keyp, &value);
	}

	// Asynchronous get
	{
		Key key(1, 1, 3);
		char *keyp = reinterpret_cast<char *>(&key);

		s = kvs->GetAsync(keyp, [&] (FogKV::KVStoreBase *kvs, FogKV::Status status, const char *key, const FogKV::KVBuff &buff) {
			if (!status.ok())
				return;
			// Done
		});
	}

	// Get range
	{
		Key beg(1, std::numeric_limits<uint16_t>::min(), 1);
		char *begp = reinterpret_cast<char *>(&beg);
		Key end(1, std::numeric_limits<uint16_t>::max(), 3);
		char *endp = reinterpret_cast<char *>(&end);
		std::vector<FogKV::KVPair> result;

		s = kvs->GetRange(begp, endp, result);

		for (auto kv: result) {
			// kv.key();
			// kv.value();
		}
	}

	// Asynchronous Get range
	{
		Key beg(1, std::numeric_limits<uint16_t>::min(), 1);
		char *begp = reinterpret_cast<char *>(&beg);
		Key end(1, std::numeric_limits<uint16_t>::max(), 3);
		char *endp = reinterpret_cast<char *>(&end);

		s = kvs->GetRangeAsync(begp, endp, [&] (FogKV::KVStoreBase *kvs, FogKV::Status status, std::vector<FogKV::KVPair> &result) {
			if (!status.ok())
				return;

			for (auto kv: result) {
				// kv.key();
				// kv.value();
			}
		});

	}
}

#if HAS_BOOST_HANA
int KVStoreExample()
{
	FogKV::Options options;

	/*
	 * key : {
	 *	size: 16,
	 *	fields : [
	 *		{
	 *			size: 	2,
	 *		},
	 *		{
	 *			size:	2,
	 *		},
	 *		{
	 *			size:	8,
	 *		},
	 *	],
	 * },
	 */
	options.Key.field(0, sizeof(Key::subdetector_id));
	options.Key.field(1, sizeof(Key::run_id));
	options.Key.field(2, sizeof(Key::event_id));

	// or with boost::hana
	FogKV::Set<Key>(options.Key);

	// Open KV store
	FogKV::Status s;
       	FogKV::KVStore<Key> *kvs = FogKV::KVStore<Key>::Open(options, &s);
	if (!s.ok()) {
		// error
		return 1;
	}

	{
		FogKV::KVBuff *buff = kvs->Alloc(256);

		std::memset(buff->data(), 0, buff->size());

		s = kvs->Put(Key(1, 1, 2), buff);
	}

	{
		std::vector<char> value;
		s = kvs->Get(Key(1, 1, 3), value);
	}
}
#endif

int main()
{
}
