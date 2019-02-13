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

#include <mutex>

#include <daqdb/KVStoreBase.h>

#include <OffloadPoller.h> /* net/if.h (put before linux/if.h) */

#include <DhtCore.h> /* include linux/if.h */
#include <PmemPoller.h>
#include <PrimaryKeyEngine.h>
#include <RTreeEngine.h>
#include <SpdkCore.h>

namespace DaqDB {

class DhtServer;

class KVStore : public KVStoreBase {
  public:
    static KVStoreBase *Open(const DaqDB::Options &options);
    virtual ~KVStore();
    void init(void);

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

    void Alloc(const char *key, size_t keySize, char **value, size_t size,
               const AllocOptions &options = AllocOptions());
    void Put(const char *key, size_t keySize, char *value, size_t valueSize,
             const PutOptions &options = PutOptions());
    void Get(const char *key, size_t keySize, char *value, size_t *valueSize,
             const GetOptions &options = GetOptions());
    void Get(const char *key, size_t keySize, char **value, size_t *valueSize,
             const GetOptions &options = GetOptions());
    void GetAny(char *key, size_t keySize,
                const GetOptions &options = GetOptions());
    void Update(const char *key, size_t keySize, char *value, size_t valueSize,
                const UpdateOptions &options = UpdateOptions());
    void Update(const char *key, size_t keySize, const UpdateOptions &options);
    void Remove(const char *key, size_t keySize);

    virtual bool IsOffloaded(Key &key);

    uint64_t GetTreeSize();
    uint64_t GetLeafCount();
    uint8_t GetTreeDepth();

    void LogMsg(std::string msg);

    inline DhtCore *getDhtCore() { return _spDht.get(); };
    inline SpdkCore *getSpdkCore() { return _spSpdk.get(); };

    inline RTreeEngine *pmem() { return _spRtree.get(); };
    inline DhtServer *dht() { return _spDhtServer.get(); };
    inline PrimaryKeyEngine *pKey() { return _spPKey.get(); };

    inline DhtServer *dhtServer() { return _spDhtServer.get(); };
    inline DhtClient *dhtClient() { return _spDht->getClient(); };

  private:
    explicit KVStore(const DaqDB::Options &options);
    inline bool isOffloadEnabled() { return getSpdkCore()->isOffloadEnabled(); }

    void _getOffloaded(const char *key, size_t keySize, char *value,
                       size_t *valueSize);
    void _getOffloaded(const char *key, size_t keySize, char **value,
                       size_t *valueSize);

    size_t _keySize;
    Options _options;

    std::unique_ptr<DhtServer> _spDhtServer;
    std::unique_ptr<RTreeEngine> _spRtree;
    std::unique_ptr<OffloadPoller> _spOffloadPoller;
    std::unique_ptr<PrimaryKeyEngine> _spPKey;
    std::vector<PmemPoller *> _rqstPollers;

    std::unique_ptr<DhtCore> _spDht;
    std::unique_ptr<SpdkCore> _spSpdk;

    std::mutex _lock;
};

} // namespace DaqDB
