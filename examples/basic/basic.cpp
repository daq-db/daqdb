/**
 * Copyright 2017-2018, Intel Corporation
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

// ![key_struct]

struct Key {
	Key() = default;
	Key(uint16_t s, uint16_t r, uint64_t e) : subdetector_id(s), run_id(r), event_id(e) {}
	uint16_t subdetector_id;
	uint16_t run_id;
	uint64_t event_id;
};

// ![key_struct]

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

int KVStoreBaseExample()
{
	// Open KV store
	//! [open]
	FogKV::Options options;

	// Configure key structure
	options.Key.field(0, sizeof(Key::subdetector_id));
	options.Key.field(1, sizeof(Key::run_id));
	options.Key.field(2, sizeof(Key::event_id));
	
	FogKV::KVStoreBase *kvs;
	try {
		kvs = FogKV::KVStoreBase::Open(options);
	} catch (FogKV::OperationFailedException &e) {
		// error
		// e.status()
	}

	// success

	//! [open]

	//! [put]
	try {
		FogKV::Key key = kvs->AllocKey();

		// Fill the key
		Key *keyp = reinterpret_cast<Key *>(key.data());
		keyp->subdetector_id = 1;
		keyp->run_id = 2;
		keyp->event_id = 3;

		FogKV::Value value = kvs->Alloc(1024);

		// Fil the value buffer
		std::memset(value.data(), 0, value.size());

		// Put operation, transfer ownership of key and value buffers to library
		kvs->Put(std::move(key), std::move(value));
	} catch (...) {
		// error 
	}
	//! [put]

	//! [put_async]
	try {
		FogKV::Key key = kvs->AllocKey();

		// Fill the key
		Key *keyp = reinterpret_cast<Key *>(key.data());
		keyp->subdetector_id = 1;
		keyp->run_id = 2;
		keyp->event_id = 3;


		FogKV::Value value = kvs->Alloc(1024);

		// Fill the value buffer
		std::memset(value.data(), 0, value.size());

		// Asynchronous Put operation, transfer ownership of key and value buffers to library
		kvs->PutAsync(std::move(key), std::move(value),
			[&] (FogKV::KVStoreBase *kvs, FogKV::Status status,
				const FogKV::Key &key, const FogKV::Value &val) {
			if (!status.ok()) {
				// error
				return;
			}

		});
	} catch (...) {
		// error 
	}
	//! [put_async]

	//! [get]
	try {
		
		FogKV::Key key = kvs->AllocKey();

		// Fill the key
		Key *keyp = reinterpret_cast<Key *>(key.data());
		keyp->subdetector_id = 1;
		keyp->run_id = 2;
		keyp->event_id = 3;


		// Get operation, the caller becomes the owner of a local copy
		// of the value. The caller must free both key and value buffers by itself, or
		// transfer the ownership in another call.
		FogKV::Value value;
		try {
			value = kvs->Get(key);
		} catch (...) {
			// error
			kvs->Free(std::move(key));
		}

		// success, process the data and free the buffers
		kvs->Free(std::move(key));
		kvs->Free(std::move(value));
	} catch (...) {
		// error 
	}
	//! [get]

	//! [get_async]
	try {
		FogKV::Key key = kvs->AllocKey();

		// Fill the key
		Key *keyp = reinterpret_cast<Key *>(key.data());
		keyp->subdetector_id = 1;
		keyp->run_id = 2;
		keyp->event_id = 3;

		try {
			kvs->GetAsync(key,
				[&] (FogKV::KVStoreBase *kvs, FogKV::Status status,
					const FogKV::Key &key, FogKV::Value value) {

				if (!status.ok()) {
					// error
					return;
				}

				// process value
				
				// free the value buffer
				kvs->Free(std::move(value));
			});
		} catch (FogKV::OperationFailedException exc) {
			// error, status in:
			// exc.status();
			kvs->Free(std::move(key));
		}
	} catch (...) {
		// error 
	}
	//! [get_async]

	{
		//! [update_value]
		// TODO
		//! [update_value]
	}

	{
		//! [update_options]
		// TODO
		//! [update_options]
	}

	{
		//! [update_async_value]
		// TODO
		//! [update_async_value]
	}

	{
		//! [update_async_options]
		// TODO
		//! [update_async_options]
	}

	{
		//! [get_range]
		// TODO
		//! [get_range]
	}

	{
		//! [get_range_async]
		// TODO
		//! [get_range_async]
	}
}

int main()
{
}
