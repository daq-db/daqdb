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

/**
 * @file
 * KVStoreBase the base class of Key-Value store.
 */

#pragma once

#include <string>
#include "FogKV/Status.h"
#include "FogKV/Options.h"
#include "FogKV/Key.h"
#include "FogKV/Value.h"
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
	/** @TODO: jradtke Temporarily using this base alias for both PUT,GET,UPDATE messages */
    using KVStoreBaseCallback = std::function<void(
        KVStoreBase *kvs, Status status, const char *key, const size_t keySize,
        const char *value, const size_t valueSize)>;

    using KVStoreBaseGetAnyCallback =
        std::function<void(KVStoreBase *kvs, Status status, const Key &key)>;
    using KVStoreBaseRangeCallback = std::function<void(
        KVStoreBase *kvs, Status status, std::vector<KVPair> &results)>;

    /**
     * Open the KV store with given options.
     *
     * @return On success, returns a pointer to a heap-allocated KV Store
     * otherwise returns nullptr
     *
     * @param[in] options Required options parameter which contains KV Store
     * configuration and runtime options.
     *
     * @snippet basic/basic.cpp open
     */
    static KVStoreBase *Open(const Options &options);

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
     * @note The ownership of key and value buffers are transferred to the
     * KVStoreBase object.
     *
     * @param[in] key Rvalue reference to key buffer.
     * @param[in] value Rvalue reference to value buffer
     * @param[in] options Put operation options.
     *
     * @throw OperationFailedException if any error occurred XXX
     *
     * @snippet basic/basic.cpp key_struct
     * @snippet basic/basic.cpp put
     */
    virtual void Put(Key &&key, Value &&value,
                     const PutOptions &options = PutOptions()) = 0;

    /**
     * Asynchronously insert a value for a given key.
     *
     * @note The ownership of key and value buffers are transferred to the
     * KVStoreBase object.
     *
     * @param[in] key Rvalue reference to key buffer.
     * @param[in] value Rvalue reference to value buffer
     * @param[in] cb Callback function. Will be called when the operation
     * completes with results passed in arguments.
     * @param[in] options Put operation options.
     *
     * @throw OperationFailedException if any error occurred XXX
     *
     * @snippet basic/basic.cpp key_struct
     * @snippet basic/basic.cpp put_async
     */
    virtual void PutAsync(Key &&key, Value &&value, KVStoreBaseCallback cb,
                          const PutOptions &options = PutOptions()) = 0;

    /**
     * Synchronously get a value for a given key.
     *
     * @return On success returns allocated buffer with value. The caller is
     * responsible of releasing the buffer.
     *
     * @param[in] key Reference to a key structure.
     * @param[in] options Get operation options.
     *
     * @throw OperationFailedException if any error occurred XXX
     *
     * @snippet basic/basic.cpp key_struct
     * @snippet basic/basic.cpp get
     */
    virtual Value Get(const Key &key,
                      const GetOptions &options = GetOptions()) = 0;

    /**
     * Synchronously get any unlocked primary key. Other fields of the key are
     * invalid.
     *
     * @return On success returns a Key of an unprocessed key.
     *
     * @param[in] options Operation options.
     *
     * @snippet basic/basic.cpp key_struct
     * @snippet basic/basic.cpp open
     * @snippet basic/basic.cpp get_any
     */
    virtual Key GetAny(const GetOptions &options = GetOptions()) = 0;

    /**
     * Asynchronously get any unlocked primary key. Other fields of the key are
     * invalid.
     *
     * @param[in] options Operation options.
     * @param[in] cb Callback function. Will be called when the operation
     * completes.
     */
    virtual void GetAnyAsync(KVStoreBaseGetAnyCallback cb,
                             const GetOptions &options = GetOptions()) = 0;

    /**
     * Asynchronously get a value for a given key.
     *
     * @param[in] key Reference to a key structure.
     * @param[in] cb Callback function. Will be called when the operation
     * completes with results passed in arguments.
     * @param[in] options Get operation options.
     *
     * @throw OperationFailedException if any error occurred XXX
     *
     * @snippet basic/basic.cpp key_struct
     * @snippet basic/basic.cpp get_async
     */
    virtual void GetAsync(const Key &key, KVStoreBaseCallback cb,
                          const GetOptions &options = GetOptions()) = 0;

    /**
     * Update value and (optionally) options for a given key.
     *
     * @note The ownership of the value buffer is transferred to the KVStoreBase
     * object.
     *
     * @param[in] key Reference to a key buffer.
     * @param[in] value Rvalue reference to a value buffer.
     * @param[in] options Update operation options.
     *
     * @throw OperationFailedException if any error occurred XXX
     *
     */
    virtual void Update(const Key &key, Value &&value,
                        const UpdateOptions &options = UpdateOptions()) = 0;

    /**
     * Update options for a given key.
     *
     * @param[in] key Reference to a key buffer.
     * @param[in] options Update operation options.
     *
     * @throw OperationFailedException if any error occurred XXX
     *
     */
    virtual void Update(const Key &key, const UpdateOptions &options) = 0;

    /**
     * Asynchronously update value and (optionally) options for a given key.
     *
     * @note The ownership of the value buffer is transferred to the KVStoreBase
     * object.
     *
     * @param[in] key Reference to a key buffer.
     * @param[in] value Rvalue reference to a value buffer.
     * @param[in] cb Callback function. Will be called when the operation
     * completes with results passed in arguments.
     * @param[in] options Update operation options.
     *
     * @throw OperationFailedException if any error occurred XXX
     *
     */
    virtual void
    UpdateAsync(const Key &key, Value &&value, KVStoreBaseCallback cb,
                const UpdateOptions &options = UpdateOptions()) = 0;

    /**
     * Asynchronously update options for a given key.
     *
     * @note The ownership of the value buffer is transferred to the KVStoreBase
     * object.
     *
     * @param[in] key Reference to a key buffer.
     * @param[in] cb Callback function. Will be called when the operation
     * completes with results passed in arguments.
     * @param[in] options Update operation options.
     *
     * @throw OperationFailedException if any error occurred XXX
     *
     */
    virtual void UpdateAsync(const Key &key, const UpdateOptions &options,
                             KVStoreBaseCallback cb) = 0;

    /**
     * Synchronously get values for a given range of keys.
     *
     * @return On success returns Ok, on failure returns a value indicating an
     * error occurred.
     *
     * @param[in] beg key representing the beginning of a range.
     * @param[in] end key representing the end of a range.
     * @param[in] options Get operation options.
     *
     * @throw OperationFailedException if any error occurred XXX
     *
     */
    virtual std::vector<KVPair>
    GetRange(const Key &beg, const Key &end,
             const GetOptions &options = GetOptions()) = 0;

    /**
     * Aynchronously get values for a given range of keys.
     *
     * @param[in] beg key representing the beginning of a range.
     * @param[in] end key representing the end of a range.
     * @param[in] cb Callback function. Will be called when the operation
     * completes with results passed in arguments.
     * @param[in] options Get operation options.
     *
     * @throw OperationFailedException if any error occurred XXX
     *
     */
    virtual void GetRangeAsync(const Key &beg, const Key &end,
                               KVStoreBaseRangeCallback cb,
                               const GetOptions &options = GetOptions()) = 0;

    /**
     * Synchronously remove a key-value store entry for a given key.
     *
     * @param[in] key Pointer to a key structure.
     *
     * @throw OperationFailedException if any error occurred XXX
     */
    virtual void Remove(const Key &key) = 0;

    /**
     * Synchronously remove key-value store entries for a given range of keys.
     *
     * @param[in] beg Pointer to a key structure representing the beginning of a
     * range.
     * @param[in] end Pointer to a key structure representing the end of a
     * range.
     *
     * @throw OperationFailedException if any error occurred XXX
     */
    virtual void RemoveRange(const Key &beg, const Key &end) = 0;

    /**
     * Allocate a Value buffer of a given size.
     *
     * @return On success returns a pointer to allocated KV buffer. Otherwise
     * returns nullptr.
     *
     * @param[in] size Size of allocation.
     * @param[in] options Allocation options.
     */
    virtual Value Alloc(const Key &key, size_t size,
                        const AllocOptions &options = AllocOptions()) = 0;

    /**
     * Deallocate a Value buffer.
     *
     * @param[in] value Value buffer. If not allocated by current instance of
     * KVStoreBase the behaviour is undefined.
     */
    virtual void Free(Value &&value) = 0;

    /**
     * Reallocate a Value buffer.
     *
     * @param[in] value KV buffer to be reallocated. If buff is nullptr this
     * call is equivalent to Alloc.
     * @param[in] size New size of a Value buffer. If the size is 0 and buff is
     * not nullptr, this call is equivalent to Free.
     * @param[in] options New options for a Value buffer.
     *
     * @throw OperationFailedException if any error occurred XXX
     *
     * @note It is possible to modify the options of an allocation without
     * changing a size, by passing the same size.
     *
     */
    virtual void Realloc(Value &value, size_t size,
                         const AllocOptions &options = AllocOptions()) = 0;

    /**
     * Change allocation options of the given Value buffer.
     *
     * @param[in] value Value buffer.
     * @param[in] options Allocation options.
     *
     * @throw OperationFailedException if any error occurred XXX
     *
     */
    virtual void ChangeOptions(Value &value, const AllocOptions &options) = 0;

    /**
     * Allocate a Key buffer
     *
     * @return On success returns a pointer to allocated Key buffer. Otherwise
     * returns nullptr.
     *
     * @param[in] options Allocation options.
     */
    virtual Key AllocKey(const AllocOptions &options = AllocOptions()) = 0;

    /**
     * Deallocate a Key buffer.
     *
     * @param[in] key Key buffer. If not allocated by current instance of
     * KVStoreBase the behavior is undefined.
     */
    virtual void Free(Key &&key) = 0;

    /**
     * Change allocation options of the given Key buffer.
     *
     * @param[in] key Key buffer.
     * @param[in] options Allocation options.
     *
     * @throw OperationFailedException if any error occurred XXX
     *
     */
    virtual void ChangeOptions(Key &key, const AllocOptions &options) = 0;
};

} // namespace FogKV
