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

#include "daqdb/KVStoreBase.h"

#include <assert.h>
#include <cstring>
#include <limits>

// ![key_struct]

struct Key {
    Key() = default;
    Key(uint64_t e, uint16_t s, uint16_t r)
        : event_id(e), subdetector_id(s), run_id(r) {}
    uint64_t event_id;
    uint16_t subdetector_id;
    uint16_t run_id;
};

// ![key_struct]

int KVStoreBaseExample() {
    // Open KV store
    //! [open]
    DaqDB::Options options;

    // Configure key structure
    options.key.field(0, sizeof(Key::event_id), true); // primary key
    options.key.field(1, sizeof(Key::subdetector_id));
    options.key.field(2, sizeof(Key::run_id));

    DaqDB::KVStoreBase *kvs;
    try {
        kvs = DaqDB::KVStoreBase::Open(options);
    } catch (DaqDB::OperationFailedException &e) {
        // error
        // e.status()
    }

    // success

    //! [open]

    //! [put]
    try {
        DaqDB::Key key = kvs->AllocKey();

        // Fill the key
        Key *keyp = reinterpret_cast<Key *>(key.data());
        keyp->subdetector_id = 1;
        keyp->run_id = 2;
        keyp->event_id = 3;

        DaqDB::Value value = kvs->Alloc(key, 1024);

        // Fil the value buffer
        std::memset(value.data(), 0, value.size());

        // Put operation, transfer ownership of key and value buffers to library
        kvs->Put(std::move(key), std::move(value));
    } catch (...) {
        // error
    }
    //! [put]

    //! [put_async]
    try {
        DaqDB::Key key = kvs->AllocKey();

        // Fill the key
        Key *keyp = reinterpret_cast<Key *>(key.data());
        keyp->subdetector_id = 1;
        keyp->run_id = 2;
        keyp->event_id = 3;

        DaqDB::Value value = kvs->Alloc(key, 1024);

        // Fill the value buffer
        std::memset(value.data(), 0, value.size());

        // Asynchronous Put operation, transfer ownership of key and value
        // buffers to library
        kvs->PutAsync(std::move(key), std::move(value),
                      [&](DaqDB::KVStoreBase *kvs, DaqDB::Status status,
                          const char *key, size_t keySize, const char *value,
                          size_t valueSize) {
                          if (!status.ok()) {
                              // error
                              return;
                          }

                      });
    } catch (...) {
        // error
    }
    //! [put_async]

    //! [get]
    try {

        DaqDB::Key key = kvs->AllocKey();

        // Fill the key
        Key *keyp = reinterpret_cast<Key *>(key.data());
        keyp->subdetector_id = 1;
        keyp->run_id = 2;
        keyp->event_id = 3;

        // Get operation, the caller becomes the owner of a local copy
        // of the value. The caller must free both key and value buffers by
        // itself, or
        // transfer the ownership in another call.
        DaqDB::Value value;
        try {
            value = kvs->Get(key);
        } catch (...) {
            // error
        }

        // success, process the data and free the buffers
        kvs->Free(key, std::move(value));
        kvs->Free(std::move(key));
    } catch (...) {
        // error
    }
    //! [get]

    //! [get_async]
    try {
        DaqDB::Key key = kvs->AllocKey();

        // Fill the key
        Key *keyp = reinterpret_cast<Key *>(key.data());
        keyp->subdetector_id = 1;
        keyp->run_id = 2;
        keyp->event_id = 3;

        try {
            kvs->GetAsync(key,
                          [&](DaqDB::KVStoreBase *kvs, DaqDB::Status status,
                              const char *key, size_t keySize,
                              const char *value, size_t valueSize) {

                              if (!status.ok()) {
                                  // error
                                  return;
                              }

                              // process value

                              // free the value buffer
                          });
        } catch (DaqDB::OperationFailedException &exc) {
            // error, status in:
            // exc.status();
            kvs->Free(std::move(key));
        }
    } catch (...) {
        // error
    }
    //! [get_async]

    //! [get_any]
    try {

        DaqDB::GetOptions getOptions;
        DaqDB::AllocOptions allocOptions;
        getOptions.attr = DaqDB::READY;
        getOptions.newAttr = DaqDB::LOCKED | DaqDB::READY;

        // get and lock any primary key which is in unlocked state
        DaqDB::Key keyBuff = kvs->GetAny(allocOptions, getOptions);
        Key *key = reinterpret_cast<Key *>(keyBuff.data());

        Key keyBeg(key->event_id, std::numeric_limits<uint16_t>::min(),
                   std::numeric_limits<uint16_t>::min());

        Key keyEnd(key->event_id, std::numeric_limits<uint16_t>::max(),
                   std::numeric_limits<uint16_t>::max());

        std::vector<DaqDB::KVPair> range = kvs->GetRange(
            DaqDB::Key(reinterpret_cast<char *>(&keyBeg), sizeof(keyBeg)),
            DaqDB::Key(reinterpret_cast<char *>(&keyEnd), sizeof(keyEnd)));

        for (auto kv : range) {
            // or unlock and mark the primary key as ready
            kvs->Update(kv.key(), DaqDB::READY);
        }

    } catch (...) {
        // error
    }
    //! [get_any]
}

int main() {}
