/**
 * Copyright 2018 Intel Corporation.
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

#include "base_operations.h"
#include "debug.h"

DaqDB::Value allocValue(std::shared_ptr<DaqDB::KVStoreBase> &spKvs,
                        const DaqDB::Key &key, const std::string &value) {
    DaqDB::Value result = spKvs->Alloc(key, value.size() + 1);
    std::memcpy(result.data(), value.c_str(), value.size());
    result.data()[result.size() - 1] = '\0';
    return result;
}

DaqDB::Key strToKey(std::shared_ptr<DaqDB::KVStoreBase> &spKvs,
                    const std::string &key) {
    DaqDB::Key keyBuff = spKvs->AllocKey();
    std::memset(keyBuff.data(), 0, keyBuff.size());
    std::memcpy(keyBuff.data(), key.c_str(), key.size());
    return keyBuff;
}

DaqDB::Value daqdb_get(std::shared_ptr<DaqDB::KVStoreBase> &spKvs,
                       const DaqDB::Key &key) {
    try {
        return spKvs->Get(key);
    } catch (DaqDB::OperationFailedException &e) {
        if (e.status()() == DaqDB::KeyNotFound) {
            BOOST_LOG_SEV(lg::get(), bt::info)
                << format("[%1%] not found") % key.data();
        } else {
            BOOST_LOG_SEV(lg::get(), bt::info)
                << "Error: cannot get element: " << e.status().to_string()
                << std::flush;
        }
    }
    return DaqDB::Value();
}

void daqdb_put(std::shared_ptr<DaqDB::KVStoreBase> &spKvs, DaqDB::Key &key,
               DaqDB::Value &val) {
    try {
        spKvs->Put(std::move(key), std::move(val));
    } catch (DaqDB::OperationFailedException &e) {
        BOOST_LOG_SEV(lg::get(), bt::info)
            << "Error: cannot put element: " << e.status().to_string()
            << std::flush;
    }
}
void daqdb_update(std::shared_ptr<DaqDB::KVStoreBase> &spKvs, DaqDB::Key &key,
                  DaqDB::Value &val) {
    try {
        spKvs->Update(std::move(key), std::move(val));
    } catch (DaqDB::OperationFailedException &e) {
        BOOST_LOG_SEV(lg::get(), bt::info)
            << "Error: cannot update element: " << e.status().to_string()
            << std::flush;
    }
}

void daqdb_remove(std::shared_ptr<DaqDB::KVStoreBase> &spKvs, DaqDB::Key &key) {

    try {
        spKvs->Remove(key);
    } catch (DaqDB::OperationFailedException &e) {
        if (e.status()() == DaqDB::KeyNotFound) {
            BOOST_LOG_SEV(lg::get(), bt::info)
                << format("[%1%] not found\n") % key.data();
        } else {
            BOOST_LOG_SEV(lg::get(), bt::info)
                << "Error: cannot remove element" << std::flush;
        }
    }
}

void daqdb_async_get(std::shared_ptr<DaqDB::KVStoreBase> &spKvs,
                     const DaqDB::Key &key,
                     DaqDB::KVStoreBase::KVStoreBaseCallback cb) {
    try {
        DaqDB::GetOptions options(DaqDB::PrimaryKeyAttribute::EMPTY);
        return spKvs->GetAsync(key, cb, std::move(options));
    } catch (DaqDB::OperationFailedException &e) {
        BOOST_LOG_SEV(lg::get(), bt::info)
            << "Error: cannot get element: " << e.status().to_string()
            << std::flush;
    }
}

void daqdb_async_put(std::shared_ptr<DaqDB::KVStoreBase> &spKvs,
                     DaqDB::Key &key, DaqDB::Value &val,
                     DaqDB::KVStoreBase::KVStoreBaseCallback cb) {
    try {
        DaqDB::PutOptions options(DaqDB::PrimaryKeyAttribute::EMPTY);
        spKvs->PutAsync(std::move(key), std::move(val), cb, std::move(options));
    } catch (DaqDB::OperationFailedException &e) {
        BOOST_LOG_SEV(lg::get(), bt::info)
            << "Error: cannot put element: " << e.status().to_string()
            << std::flush;
    }
}

void daqdb_async_update(std::shared_ptr<DaqDB::KVStoreBase> &spKvs,
                        DaqDB::Key &key, DaqDB::Value &val,
                        DaqDB::KVStoreBase::KVStoreBaseCallback cb) {
    try {
        spKvs->UpdateAsync(std::move(key), std::move(val), cb);
    } catch (DaqDB::OperationFailedException &e) {
        BOOST_LOG_SEV(lg::get(), bt::info)
            << "Error: cannot update element: " << e.status().to_string()
            << std::flush;
    }
}
