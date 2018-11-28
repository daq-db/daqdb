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

#include <KVStoreThin.h>
#include <daqdb/KVStoreBase.h>

#ifndef THIN_LIB
#include <KVStore.h>
#endif

namespace DaqDB {

KVStoreBase *KVStoreBase::Open(const Options &options) {

    if (options.mode == OperationalMode::STORAGE) {
#ifndef THIN_LIB
        return KVStore::Open(options);
#else
        throw OperationFailedException();
#endif

    } else {
        return KVStoreThin::Open(options);
    }
}

} // namespace DaqDB
