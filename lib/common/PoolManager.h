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

#include <map>

using namespace std;

#include "GeneralPoolBase.h"

#include <mutex>
#include <shared_mutex>
#include <thread>

typedef std::mutex Lock;
typedef std::unique_lock<Lock> WriteLock;
typedef std::unique_lock<Lock> ReadLock;

namespace MemMgmt {
class PoolManager {
  public:
    typedef map<unsigned int, GeneralPoolBase *, less<unsigned int>>
        GeneralPoolMap;

  public:
    ~PoolManager();
    static void start(PoolManager *arg);
    static PoolManager &getInstance();
    void registerPool(GeneralPoolBase *pool_);
    void unregisterPool(GeneralPoolBase *pool_);
    void dump();

#ifdef _MEM_STATS_
    char *printDateTime(const time_t &dtm);
#endif

  private:
    PoolManager();
    // PoolManager(const PoolManager &right);
    // PoolManager & operator=(const PoolManager &right);

    static PoolManager *instance;
    Lock mutex;
    GeneralPoolMap pools;
    static Lock instMutex;

    std::thread *collector;

#ifdef _MEM_STATS_
    FILE *statsFile;
    int statsInterval;
    bool statsOn;
#endif
};
} // namespace MemMgmt
