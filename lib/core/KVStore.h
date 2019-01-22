/**
 * Copyright 2017-2018 Intel Corporation.
 *
 * This software and the related documents are Intel copyrighted materials,
 * and your use of them is governed by the express license under which they
 * were provided to you (Intel OBL Internal Use License).
 * Unless the License provides otherwise, you may not use, modify, copy,
 * publish, distribute, disclose or transmit this software or the related
 * documents without Intel's prior written permission.
 *
 * This software and the related documents are provided as is, with no
 * express or implied warranties, other than those that are expressly
 * stated in the License.
 */

#pragma once

#include <mutex>

#include <asio/io_service.hpp>

#include <daqdb/KVStoreBase.h>

#include <DhtServer.h>
#include <OffloadPoller.h>
#include <PmemPoller.h>
#include <PrimaryKeyEngine.h>
#include <RTreeEngine.h>
#include <SpdkCore.h>

namespace DaqDB {

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
    virtual Key GetAny(const GetOptions &options = GetOptions());
    virtual void GetAnyAsync(KVStoreBaseGetAnyCallback cb,
                             const GetOptions &options = GetOptions());
    virtual void Remove(const Key &key);
    virtual void RemoveRange(const Key &beg, const Key &end);
    virtual Value Alloc(const Key &key, size_t size,
                        const AllocOptions &options = AllocOptions());
    virtual void Free(const Key &key, Value &&value);
    virtual void Realloc(Value &value, size_t size,
                         const AllocOptions &options = AllocOptions());
    virtual void ChangeOptions(Value &value, const AllocOptions &options);
    virtual Key AllocKey(const AllocOptions &options = AllocOptions());
    virtual void Free(Key &&key);
    virtual void ChangeOptions(Key &key, const AllocOptions &options);

    virtual bool IsOffloaded(Key &key);

    void LogMsg(std::string msg);

    inline DhtCore *getDhtCore() { return _spDht.get(); };
    inline SpdkCore *getSpdkCore() { return _spSpdk.get(); };

    inline RTreeEngine *pmem() { return _spRtree.get(); };
    inline DhtServer *dht() { return _spDhtServer.get(); };
    inline PrimaryKeyEngine *pKey() { return _spPKey.get(); };

    inline DhtServer *dhtServer() { return _spDhtServer.get(); };
    inline DhtClient *dhtClient() { return _spDht->getClient(); };

    inline asio::io_service &getIoService() { return _ioService; };

  private:
    explicit KVStore(const DaqDB::Options &options);

    void _put(const char *key, size_t keySize, char *value, size_t valueSize,
              const PutOptions &options = PutOptions());
    void _get(const char *key, size_t keySize, char *value, size_t *valueSize,
              const GetOptions &options = GetOptions());
    void _get(const char *key, size_t keySize, char **value, size_t *valueSize,
              const GetOptions &options = GetOptions());
    void _getOffloaded(const char *key, size_t keySize, char *value,
                       size_t *valueSize);
    void _getOffloaded(const char *key, size_t keySize, char **value,
                       size_t *valueSize);
    void _update(const char *key, size_t keySize, char *value, size_t valueSize,
                 const UpdateOptions &options = UpdateOptions());
    void _update(const char *key, size_t keySize, const UpdateOptions &options);
    void _remove(const char *key, size_t keySize);
    inline bool isOffloadEnabled() { return getSpdkCore()->isOffloadEnabled(); }

    asio::io_service _ioService;
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
