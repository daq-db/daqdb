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

#include "common.h"
#include "debug.h"

DaqDB::Value allocValue(DaqDB::KVStoreBase *kvs, const DaqDB::Key &key,
                        const std::string &value) {
    DaqDB::Value result = kvs->Alloc(key, value.size() + 1);
    std::memcpy(result.data(), value.c_str(), value.size());
    result.data()[result.size() - 1] = '\0';
    return result;
}

DaqDB::Key strToKey(DaqDB::KVStoreBase *kvs, const std::string &key) {
    DaqDB::Key keyBuff = kvs->AllocKey();
    std::memset(keyBuff.data(), 0, keyBuff.size());
    std::memcpy(keyBuff.data(), key.c_str(), key.size());
    return keyBuff;
}

DaqDB::Value remote_get(DaqDB::KVStoreBase *kvs, const DaqDB::Key &key) {
    try {
        return kvs->Get(key);
    } catch (DaqDB::OperationFailedException &e) {
        if (e.status()() == DaqDB::KEY_NOT_FOUND) {
            LOG_INFO << format("[%1%] not found") % key.data();
        } else {
            LOG_INFO << "Error: cannot get element: " << e.status().to_string()
                     << std::flush;
        }
    }
    return DaqDB::Value();
}

void remote_put(DaqDB::KVStoreBase *kvs, DaqDB::Key &key, DaqDB::Value &val) {
    try {
        kvs->Put(std::move(key), std::move(val));
    } catch (DaqDB::OperationFailedException &e) {
        LOG_INFO << "Error: cannot put element: " << e.status().to_string()
                 << std::flush;
    }
}
