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

#include <daqdb/KVStoreBase.h>

#ifndef THIN_LIB
#include <KVStore.h> /* net/if.h (put before linux/if.h) */
#endif

#include <KVStoreThin.h> /* include linux/if.h */

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
