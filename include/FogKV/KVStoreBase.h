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

/**
 * @file
 * KVStoreBase the base class of Key-Value store.
 */

#pragma once

#include <string>
#include "FogKV/Status.h"
#include "FogKV/Options.h"
#include "FogKV/KVBuff.h"
#include "FogKV/KVPair.h"
#include <functional>

namespace FogKV {

/**
 * The KVStoreBase is a distributed Key-Value store.
 *
 * The key size is constant and the value is variadic length non-null
 * terminated stream of bytes.
 *
 * This is a non-template class version of KV store, in which both key and
 * value are represented by (char *) type.
 *
 * This class is safe for concurrent use from multiple threads without a need
 * for external synchronization.
 *
 */
class KVStoreBase {
public:
	using KVStoreBaseCallback = std::function<void (KVStoreBase *kvs, Status status, const char *key, const KVBuff &buff)>;
	using KVStoreBaseRangeCallback = std::function<void (KVStoreBase *kvs, Status status, std::vector<KVPair> &results)>;

	/**
	 * Open the KV store with given options.
	 *
	 * @return On success, returns a pointer to a heap-allocated KV Store
	 * otherwise returns nullptr
	 *
	 * @param[in] options Required options parameter which contains KV Store
	 * configuration and runtime options.
	 * @param[out] status If not nullptr, will contain a status code if error
	 * occured. Otherwise Ok value is stored.
	 *
	 * @snippet basic/basic.cpp open
	 */
	static KVStoreBase *Open(const Options &options, Status *status);
public:

	/**
	 * Return the size of a key, which the KV store uses.
	 *
	 * @return Key size in bytes
	 */
	virtual size_t KeySize() = 0;

	/**
	 * Return the configuration and runtime options which the KV store
	 * has been opened with.
	 *
	 * @return Configuration and runtime options
	 */
	virtual const Options &getOptions() = 0;

	/**
	 * Return a given property for the KV store.
	 *
	 * XXX full documentation of supported properties
	 */
	virtual std::string getProperty(const std::string &name) = 0;

	/**
	 * Synchronously insert a value for a given key. 
	 *
	 * @return On success returns Ok, on failure returns a value indicating an error occurred.
	 *
	 * @param[in] key Pointer to a key structure.
	 * @param[in] value Pointer to a value.
	 * @param[in] size Size of value.
	 *
	 * @snippet basic/basic.cpp key_struct 
	 * @snippet basic/basic.cpp put_sync_char
	 */
	virtual Status Put(const char *key, const char *value, size_t size) = 0;

	/**
	 * Synchronously insert a value for a given key. 
	 *
	 * @return On success returns Ok, on failure returns a value indicating an error occurred.
	 *
	 * @param[in] key Pointer to a key structure.
	 * @param[in] buff Pointer to Value buffer structure. The buff cannot be used anymore.
	 *
	 * @snippet basic/basic.cpp key_struct 
	 * @snippet basic/basic.cpp put_sync_buff
	 */
	virtual Status Put(const char *key, KVBuff *buff) = 0;

	/**
	 * Asynchronously insert a value for a given key.
	 *
	 * @return On success returns Ok, on failure returns a value indicating an error occurred.
	 *
	 * @param[in] key Pointer to a key structure.
	 * @param[in] value Pointer to a value.
	 * @param[in] size Size of value.
	 * @param[in] cb Callback function. Will be called when the operation completes.
	 *
	 * @snippet basic/basic.cpp key_struct 
	 * @snippet basic/basic.cpp put_async_char
	 */
	virtual Status PutAsync(const char *key, const char *value, size_t size, KVStoreBaseCallback cb) = 0;

	/**
	 * Asynchronously insert a value for a given key. 
	 *
	 * @return On success returns Ok, on failure returns a value indicating an error occurred.
	 *
	 * @param[in] key Pointer to a key structure.
	 * @param[in] buff Pointer to Value buffer structure. The buff cannot be used anymore.
	 * @param[in] cb Callback function. Will be called when the operation completes with results passed in arguments.
	 *
	 * @snippet basic/basic.cpp key_struct 
	 * @snippet basic/basic.cpp put_async_buff
	 */
	virtual Status PutAsync(const char *key, KVBuff *buff, KVStoreBaseCallback cb) = 0;

	/**
	 * Synchronously get a value for a given key.
	 *
	 * @return On success returns Ok, on failure returns a value indicating an error occurred.
	 *
	 * @param[in] key Pointer to a key structure.
	 * @param[out] value Vector of bytes with value for a given key.
	 *
	 * @snippet basic/basic.cpp key_struct 
	 * @snippet basic/basic.cpp get_sync_vector
	 */
	virtual Status Get(const char *key, std::vector<char> &value) = 0;

	/**
	 * Synchronously get a value for a given key.
	 *
	 * @return On success returns Ok, on failure returns a value indicating an error occurred.
	 *
	 * @param[in] key Pointer to a key structure.
	 * @param[out] value Pointer to a pointer to a key-Value buffer structure. On success
	 * the buffer is allocated. The caller is responsible of releasing the buffer using Free method.
	 * Otherwise the buff is left untouched.
	 *
	 * @snippet basic/basic.cpp key_struct 
	 * @snippet basic/basic.cpp get_sync_buff_ptr
	 */
	virtual Status Get(const char *key, KVBuff **value) = 0;

	/**
	 * Asynchronously get a value for a given key.
	 *
	 * @return On success returns Ok, on failure returns a value indicating an error occurred.
	 *
	 * @param[in] key Pointer to a key structure.
	 * @param[in] cb Callback function. Will be called when the operation completes with results passed in arguments.
	 *
	 * @snippet basic/basic.cpp key_struct 
	 * @snippet basic/basic.cpp get_async
	 */
	virtual Status GetAsync(const char *key, KVStoreBaseCallback cb) = 0;

	/**
	 * Synchronously get values for a given range of keys.
	 *
	 * @return On success returns Ok, on failure returns a value indicating an error occurred.
	 *
	 * @param[in] beg Pointer to a key structure representing the beginning of a range.
	 * @param[in] end Pointer to a key structure representing the end of a range.
	 * @param[out] result Vector of KVPair elements each representing a key and a value found for a given range.
	 *
	 * @snippet basic/basic.cpp key_struct 
	 * @snippet basic/basic.cpp get_range_sync
	 */
	virtual Status GetRange(const char *beg, const char *end, std::vector<KVPair> &result) = 0;

	/**
	 * Aynchronously get values for a given range of keys.
	 *
	 * @return On success returns Ok, on failure returns a value indicating an error occurred.
	 *
	 * @param[in] beg Pointer to a key structure representing the beginning of a range.
	 * @param[in] end Pointer to a key structure representing the end of a range.
	 * @param[in] cb Callback function. Will be called when the operation completes with results passed in arguments.
	 *
	 * @snippet basic/basic.cpp key_struct 
	 * @snippet basic/basic.cpp get_range_async
	 */
	virtual Status GetRangeAsync(const char *beg, const char *end, KVStoreBaseRangeCallback cb) = 0;

	/**
	 * Synchronously remove a value for a given key.
	 *
	 * @return On success returns Ok, on failure returns a value indicating an error occurred.
	 *
	 * @param[in] key Pointer to a key structure.
	 */
	virtual Status Remove(const char *key) = 0;

	/**
	 * Synchronously remove values for a given range of key.
	 *
	 * @return On success returns Ok, on failure returns a value indicating an error occurred.
	 *
	 * @param[in] beg Pointer to a key structure representing the beginning of a range.
	 * @param[in] end Pointer to a key structure representing the end of a range.
	 */
	virtual Status RemoveRange(const char *beg, const char *end) = 0;

	/**
	 * Allocate a Value buffer of a given size.
	 *
	 * @return On success returns a pointer to allocated KV buffer. Otherwise returns nullptr.
	 *
	 * @param[in] size Size of allocation.
	 * @param[in] options Allocation options.
	 */
	virtual KVBuff *Alloc(size_t size, const AllocOptions &options = AllocOptions()) = 0;

	/**
	 * Deallocate a Value buffer.
	 *
	 * @param[in] buff KV buffer. If not allocated by current instance of KVStoreBase the behaviour is undefined.
	 */
	virtual void Free(KVBuff *buff) = 0;

	/**
	 * Reallocate a Value buffer.
	 *
	 * @returns On success returns a pointer to reallocated KV buffer and this can be different than buff. Otherwise returns nullptr.
	 *
	 * @param[in] buff KV buffer to be reallocated. If buff is nullptr this call is equivalent to Alloc.
	 * @param[in] size New size of a KV buffer. If the size is 0 and buff is not nullptr, this call is equivalent to Free.
	 * @param[in] options New options for a KV buffer.
	 *
	 * @note It is possible to modify the options of an allocation without changing a size, by passing the same size.
	 *
	 */
	virtual KVBuff *Realloc(KVBuff *buff, size_t size, const AllocOptions &options = AllocOptions()) = 0;
};

} // namespace FogKV
