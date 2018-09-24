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

#include <OffloadReactor.h>
#include <OffloadPooler.h>
#include <RTreeEngine.h>
#include <RqstPooler.h>
#include <daqdb/KVStoreBase.h>
#include <dht/ZhtNode.h>
#include <dht/DhtNode.h>
#include <mutex>

namespace DaqDB {

class KVStoreBaseImpl : public KVStoreBase {
  public:
    static KVStoreBase *Open(const Options &options);

  public:
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
    virtual void Free(Value &&value);
    virtual void Realloc(Value &value, size_t size,
                         const AllocOptions &options = AllocOptions());
    virtual void ChangeOptions(Value &value, const AllocOptions &options);
    virtual Key AllocKey(const AllocOptions &options = AllocOptions());
    virtual void Free(Key &&key);
    virtual void ChangeOptions(Key &key, const AllocOptions &options);

    void LogMsg(std::string msg);

  protected:
    explicit KVStoreBaseImpl(const Options &options);
    virtual ~KVStoreBaseImpl();

    void init();
    void registerProperties();

    asio::io_service &io_service();
    asio::io_service *_io_service;

    size_t mKeySize;
    Options mOptions;
    std::unique_ptr<DaqDB::DhtNode> mDhtNode;
    std::shared_ptr<DaqDB::RTreeEngine> mRTree;
    std::mutex mLock;

    std::vector<RqstPooler *> _rqstPoolers;

    OffloadReactor *_offloadReactor;
    OffloadPooler *_offloadPooler;
    bool _offloadEnabled = false;
};

}
