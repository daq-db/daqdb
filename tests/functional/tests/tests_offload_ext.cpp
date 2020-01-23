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

#include <chrono>
#include <condition_variable>
#include <random>

#include "tests.h"
#include <base_operations.h>
#include <debug.h>

using namespace std::chrono_literals;
using namespace std;
using namespace DaqDB;

/*
 * Insert multiple tuples with varying data sizes
 * into kvs. Do data integrity check afterwards.
 */

class KVSet64 {
  public:
    KVSet64() = default;
    virtual ~KVSet64() = default;

    void generateUniformIntNoDups(size_t setSize, uint64_t maxVal = (4096 * 7));
    bool operator==(const KVSet64 &r);
    bool operator!=(const KVSet64 &r);
    void addKv(const pair<uint64_t, Value> &kv);
    void clearAll();

  protected:
    Value generateValue(default_random_engine &gen,
                        uniform_int_distribution<uint64_t> &dist,
                        uint64_t maxVal);
    uint64_t generateKeyNoDup(default_random_engine &gen,
                              uniform_int_distribution<uint64_t> &dist);

  public:
    vector<pair<uint64_t, Value>> kvpairs;
};

void KVSet64::addKv(const pair<uint64_t, Value> &kv) { kvpairs.push_back(kv); }

void KVSet64::clearAll() { kvpairs.clear(); }

uint64_t KVSet64::generateKeyNoDup(default_random_engine &gen,
                                   uniform_int_distribution<uint64_t> &dist) {
    uint64_t keyCandidate = 0;
    bool noDups = false;
    while (noDups == false) {
        keyCandidate = dist(gen);
        bool foundDup = false;
        for (size_t k = 0; k < kvpairs.size(); k++) {
            if (keyCandidate == kvpairs[k].first) {
                foundDup = true;
                break;
            }
        }
        if (foundDup == false)
            break;
        foundDup = false;
    }
    return keyCandidate;
}

Value KVSet64::generateValue(default_random_engine &gen,
                             uniform_int_distribution<uint64_t> &dist,
                             uint64_t maxVal) {
    size_t actValSize = dist(gen) % maxVal;
    Value val(new char[actValSize], actValSize);

    size_t loopCnt = val.size() / sizeof(uint64_t);
    for (uint64_t j = 0; j < loopCnt; j++) {
        uint64_t jVal = dist(gen);
        *reinterpret_cast<uint64_t *>(val.data() + j * sizeof(uint64_t)) = jVal;
    }
    return val;
}

void KVSet64::generateUniformIntNoDups(size_t setSize, uint64_t maxVal) {
    default_random_engine gen;
    uniform_int_distribution<uint64_t> dist(0, -1);

    kvpairs.resize(setSize);

    for (size_t i = 0; i < setSize; i++) {
        kvpairs[i].first = generateKeyNoDup(gen, dist);
        kvpairs[i].second = generateValue(gen, dist, maxVal);
    }
}

bool KVSet64::operator==(const KVSet64 &r) {
    if (kvpairs.size() != r.kvpairs.size()) {
        DAQDB_INFO
            << format("Error: wrong result ref.size[%1%] == res.size[%2%]") %
                   kvpairs.size() % r.kvpairs.size();
        return false;
    }

    for (auto &tr : r.kvpairs) {
        bool keyFound = false;
        for (auto &tl : kvpairs) {
            if (tr.first == tl.first) {
                keyFound = true;
                if (tr.second.size() != tl.second.size() ||
                    memcmp(tr.second.data(), tl.second.data(),
                           tl.second.size())) {
                    DAQDB_INFO << format("Error: wrong value ref.val.size[%1%] "
                                         "== res.val.size[%2%]") %
                                      tr.second.size() % tl.second.size();
                    return false;
                }
                break;
            }
        }
        if (keyFound == false) {
            const char *keyData = reinterpret_cast<const char *>(&tr.first);
            Key key(const_cast<char *>(keyData), sizeof(tr.first));
            DAQDB_INFO << format("Error: key not found ref.key[%1%]") %
                              keyToStr(key);
            return false;
        }
    }
    return true;
}

bool KVSet64::operator!=(const KVSet64 &r) { return !operator==(r); }

KVSet64 kvsetRef; // reference kv set
bool initialized = false;

static void InitKVRefSet() {
    kvsetRef.generateUniformIntNoDups(64); // generate random 64 kv pairs
}
/*
 * Run insert a number of times and compare results
 */
bool testSyncOffloadExtOperations(KVStoreBase *kvs) {
    bool result = true;

    if (initialized == false) {
        InitKVRefSet();
        initialized = true;
    }
    auto &kvpRef = kvsetRef.kvpairs;

    for (auto &kv : kvpRef) {
        daqdb_put_value(kvs, kv.first, kv.second);
    }

    KVSet64 kvsetRes; // result kv set
    for (auto &kv : kvpRef) {
        auto resultVal = daqdb_get(kvs, kv.first);
        kvsetRes.addKv(pair<uint64_t, Value>(kv.first, resultVal));
    }

    if (kvsetRef != kvsetRes) {
        DAQDB_INFO << "Error: corrupt data after put/get";
        result = false;
    }

    for (auto &kv : kvpRef) {
        auto key = allocKey(kvs, kv.first);
        if (kvs->IsOffloaded(key) == true) {
            DAQDB_INFO << "Error: wrong value location";
            result = false;
        }
    }

    for (auto &kv : kvpRef) {
        daqdb_offload(kvs, kv.first);
    }

    for (auto &kv : kvpRef) {
        auto key = allocKey(kvs, kv.first);
        if (kvs->IsOffloaded(key) == false) {
            DAQDB_INFO << "Error: wrong value location";
            result = false;
        }
    }

    kvsetRes.clearAll();
    for (auto &kv : kvpRef) {
        auto resultVal = daqdb_get(kvs, kv.first);
        kvsetRes.addKv(pair<uint64_t, Value>(kv.first, resultVal));
    }

    if (kvsetRef != kvsetRes) {
        DAQDB_INFO << "Error: wrong data after offload";
        result = false;
    }

    kvsetRes.clearAll();
    for (auto &kv : kvpRef) {
        auto removeResult = daqdb_remove(kvs, kv.first);
        if (removeResult == false) {
            result = false;
            DAQDB_INFO << format("Error: Cannot remove a key [%1%]") % kv.first;
            kvsetRes.addKv(pair<uint64_t, Value>(kv.first, kv.second));
        }
    }

    if (kvsetRes.kvpairs.size()) {
        DAQDB_INFO << "Error: wrong data after remove all";
        result = false;
    }

    return result;
}

bool testAsyncOffloadExtOperations(KVStoreBase *kvs) {
    bool result = true;

    if (initialized == false) {
        InitKVRefSet();
        initialized = true;
    }
    auto &kvpRef = kvsetRef.kvpairs;

    mutex mtx;
    condition_variable cv;
    bool ready = false;

    for (auto &kv : kvpRef) {
        daqdb_async_put_value(
            kvs, kv.first, kv.second,
            [&](KVStoreBase *kvs, Status status, const char *argKey,
                const size_t keySize, const char *value,
                const size_t valueSize) {
                unique_lock<mutex> lck(mtx);
                if (status.ok()) {
                    DAQDB_INFO
                        << boost::format("PutAsync: [%1%]") % keyToStr(argKey);
                } else {
                    DAQDB_INFO
                        << boost::format("Error: cannot put element: %1%") %
                               status.to_string();
                    result = false;
                }
                ready = true;
                cv.notify_all();
            });

        // wait for completion
        {
            unique_lock<mutex> lk(mtx);
            cv.wait_for(lk, 1s, [&ready] { return ready; });
            ready = false;
        }
    }

    for (auto &kv : kvpRef) {
        auto key = allocKey(kvs, kv.first);
        if (kvs->IsOffloaded(key) == true) {
            DAQDB_INFO << "Error: wrong value location";
            result = false;
        }
    }

    KVSet64 kvsetRes; // result kv set
    for (auto &kv : kvpRef) {
        auto resultVal = daqdb_get(kvs, kv.first);
        kvsetRes.addKv(pair<uint64_t, Value>(kv.first, resultVal));
    }

    if (kvsetRef != kvsetRes) {
        DAQDB_INFO << "Error: corrupt data after put/get";
        result = false;
    }

    kvsetRes.clearAll();
    for (auto &kv : kvpRef) {
        daqdb_async_get(
            kvs, kv.first,
            [&](KVStoreBase *kvs, Status status, const char *argKey,
                size_t keySize, const char *value, size_t valueSize) {
                unique_lock<mutex> lck(mtx);

                if (status.ok()) {
                    DAQDB_INFO << boost::format("GetAsync: [%1%] = %2%") %
                                      keyToStr(argKey) % valueSize;
                    uint64_t ukey = *reinterpret_cast<const uint64_t *>(argKey);
                    Value uval(const_cast<char *>(value), valueSize);
                    kvsetRes.addKv(pair<uint64_t, Value>(ukey, uval));
                } else {
                    DAQDB_INFO
                        << boost::format("Error: cannot get element: %1%") %
                               status.to_string();
                    result = false;
                }

                ready = true;
                cv.notify_all();
            });

        // wait for completion
        {
            unique_lock<mutex> lk(mtx);
            cv.wait_for(lk, 1s, [&ready] { return ready; });
            ready = false;
        }
    }

    if (kvsetRef != kvsetRes) {
        DAQDB_INFO << "Error: corrupt data after put/get_async";
        result = false;
    }

    for (auto &kv : kvpRef) {
        daqdb_async_offload(
            kvs, kv.first,
            [&](KVStoreBase *kvs, Status status, const char *argKey,
                const size_t keySize, const char *value,
                const size_t valueSize) {
                unique_lock<mutex> lck(mtx);
                if (status.ok()) {
                    DAQDB_INFO
                        << boost::format("Offload: [%1%]") % keyToStr(argKey);
                } else {
                    DAQDB_INFO << boost::format("Offload Error: %1%") %
                                      status.to_string();
                    result = false;
                }
                ready = true;
                cv.notify_all();
            });

        // wait for completion
        {
            unique_lock<mutex> lk(mtx);
            cv.wait_for(lk, 1s, [&ready] { return ready; });
            ready = false;
        }
    }

    for (auto &kv : kvpRef) {
        auto key = allocKey(kvs, kv.first);
        if (kvs->IsOffloaded(key) == false) {
            DAQDB_INFO << "Error: wrong value location";
            result = false;
        }
    }

    kvsetRes.clearAll();
    for (auto &kv : kvpRef) {
        auto resultVal = daqdb_get(kvs, kv.first);
        kvsetRes.addKv(pair<uint64_t, Value>(kv.first, resultVal));
    }

    if (kvsetRef != kvsetRes) {
        DAQDB_INFO << "Error: corrupt data after put/get_async/get";
        result = false;
    }

    kvsetRes.clearAll();
    for (auto &kv : kvpRef) {
        daqdb_async_get(
            kvs, kv.first,
            [&](KVStoreBase *kvs, Status status, const char *argKey,
                size_t keySize, const char *value, size_t valueSize) {
                unique_lock<mutex> lck(mtx);

                if (status.ok()) {
                    DAQDB_INFO << boost::format("GetAsync: [%1%] = %2%") %
                                      keyToStr(argKey) % valueSize;
                    uint64_t ukey = *reinterpret_cast<const uint64_t *>(argKey);
                    Value uval(const_cast<char *>(value), valueSize);
                    kvsetRes.addKv(pair<uint64_t, Value>(ukey, uval));
                } else {
                    DAQDB_INFO
                        << boost::format("Error: cannot get element: %1%") %
                               status.to_string();
                    result = false;
                }

                ready = true;
                cv.notify_all();
            });

        // wait for completion
        {
            unique_lock<mutex> lk(mtx);
            cv.wait_for(lk, 1s, [&ready] { return ready; });
            ready = false;
        }
    }

    if (kvsetRef != kvsetRes) {
        DAQDB_INFO << "Error: corrupt data after put/async_offload/async_get";
        result = false;
    }

    kvsetRes.clearAll();
    for (auto &kv : kvpRef) {
        auto removeResult = daqdb_remove(kvs, kv.first);
        if (removeResult == false) {
            result = false;
            DAQDB_INFO << format("Error: Cannot remove a key [%1%]") % kv.first;
            kvsetRes.addKv(pair<uint64_t, Value>(kv.first, kv.second));
        }
    }

    if (kvsetRes.kvpairs.size()) {
        DAQDB_INFO << "Error: wrong data after put/async_offload/remove";
        result = false;
    }

    return result;
}
