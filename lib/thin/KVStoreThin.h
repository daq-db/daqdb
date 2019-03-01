/**
 *  Copyright (c) 2019 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <DhtCore.h>
#include <daqdb/KVStoreBase.h>

namespace DaqDB {

class KVStoreThin : public KVStoreBase {
  public:
    static KVStoreBase *Open(const DaqDB::Options &options);

    virtual size_t KeySize();
    virtual const Options &getOptions();
    virtual std::string getProperty(const std::string &name);
    virtual void Put(Key &&key, Value &&value,
                     const PutOptions &options = PutOptions());
    virtual void PutAsync(Key &&key, Value &&value, KVStoreBaseCallback cb,
                          const PutOptions &options = PutOptions());
    virtual Value Get(const Key &key, const GetOptions &options = GetOptions());
    virtual void GetAsync(const Key &key, KVStoreBaseCallback cb,
                          const GetOptions &options = GetOptions());
    virtual void Update(const Key &key, Value &&value,
                        const UpdateOptions &options = UpdateOptions());
    virtual void Update(const Key &key, const UpdateOptions &options);
    virtual void UpdateAsync(const Key &key, Value &&value,
                             KVStoreBaseCallback cb,
                             const UpdateOptions &options = UpdateOptions());
    virtual void UpdateAsync(const Key &key, const UpdateOptions &options,
                             KVStoreBaseCallback cb);
    virtual std::vector<KVPair>
    GetRange(const Key &beg, const Key &end,
             const GetOptions &options = GetOptions());
    virtual void GetRangeAsync(const Key &beg, const Key &end,
                               KVStoreBaseRangeCallback cb,
                               const GetOptions &options = GetOptions());
    virtual Key GetAny(const AllocOptions &allocOptions = AllocOptions(),
                       const GetOptions &options = GetOptions());
    virtual void GetAnyAsync(KVStoreBaseGetAnyCallback cb,
                             const AllocOptions &allocOptions = AllocOptions(),
                             const GetOptions &options = GetOptions());
    virtual void Remove(const Key &key);
    virtual void RemoveRange(const Key &beg, const Key &end);
    virtual Value Alloc(const Key &key, size_t size,
                        const AllocOptions &options = AllocOptions());
    virtual void Free(const Key &key, Value &&value);
    virtual void Realloc(const Key &key, Value &value, size_t size,
                         const AllocOptions &options = AllocOptions());
    virtual void ChangeOptions(Value &value, const AllocOptions &options);
    virtual Key AllocKey(const AllocOptions &options = AllocOptions());
    virtual void Free(Key &&key);
    virtual void ChangeOptions(Key &key, const AllocOptions &options);

    virtual bool IsOffloaded(Key &key);

    virtual uint64_t GetTreeSize();
    virtual uint64_t GetLeafCount();
    virtual uint8_t GetTreeDepth();

    void LogMsg(std::string msg);

    inline DaqDB::DhtCore *getDhtCore() { return _spDht.get(); };

    inline DhtClient *dhtClient() { return _spDht->getClient(); };

  private:
    explicit KVStoreThin(const DaqDB::Options &options);
    virtual ~KVStoreThin();

    void init();

    size_t _keySize;
    Options _options;

    std::unique_ptr<DhtCore> _spDht;
};

} // namespace DaqDB
