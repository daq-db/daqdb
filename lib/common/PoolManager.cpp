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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "PoolManager.h"

namespace DaqDB {
PoolManager *PoolManager::instance = 0;
Lock PoolManager::instMutex;

PoolManager::PoolManager()
    : collector(0)
#ifdef _MEM_STATS_
      ,
      statsFile(0), statsInterval(defaultStatsInterval), statsOn(false)
#endif
{
#ifdef _MEM_STATS_
    // get stats switch
    char *sf = getenv("MEM_STATS");
    if (sf) {
        if (atoi(sf) == 1) {
            // open stats file
            char name_buf[64];
            sprintf(name_buf, "mem_mgmt_stats_%d", getpid());
            statsFile = fopen(name_buf, "w");

            // check if OK
            if (statsFile)
                statsOn = true;
        }
    }

    // get interval
    char *si = getenv("MEM_STATS_INTERVAL");
    if (si) {
        statsInterval = atoi(si);
    }
#endif
}

PoolManager::~PoolManager() {
    // users must implement an outside synchronization mechanism to create and
    // delete this singleton such that nobody is left with instance that has
    // been deleted. This code only makes instance deletion atomic.
    WriteLock w_lock(PoolManager::instMutex);
    PoolManager::instance = 0;
}

void PoolManager::start(PoolManager *arg) {
    for (;;) {
        // sleep a little bit
#ifdef _MEM_STATS_
        if (arg->statsInterval < defaultStatsInterval && arg->statsOn == true)
            sleep(arg->statsInterval);
        else
            ntervalsleep(defaultSleep);
#else
        sleep(defaultSleepInterval);
#endif

        {
            // lock the map
            WriteLock w_lock(arg->mutex);

            // manage pools
            for (GeneralPoolMap::iterator it = arg->pools.begin();
                 it != arg->pools.end(); it++) {
                (*it).second->manage();
            }
        }

        sleep(1);

        {
            // lock the map
            WriteLock w_lock(arg->mutex);

#ifdef _MEM_STATS_
            if (arg->statsOn == true) {
                // produce stats
                for (GeneralPoolMap::iterator it = arg->pools.begin();
                     it != arg->pools.end(); it++) {
                    time_t ct = time(0);
                    fprintf(arg->statsFile, "%s ", arg->printDateTime(ct));
                    (*it).second->stats(*arg->statsFile);
                }

                // flush
                fflush(arg->statsFile);
            }
#endif
        }
    }

    return;
}

PoolManager &PoolManager::getInstance() {
    // users must implement an outside synchronization mechanism to create and
    // delete this singleton such that nobody is left with instance that has
    // been deleted. This code only makes instance cration atomic.
    WriteLock w_lock(PoolManager::instMutex);

    // get the instance and get it going
    if (!instance) {
        instance = new PoolManager();
        instance->collector = new std::thread(&PoolManager::start, instance);
    }

    return *instance;
}

void PoolManager::registerPool(GeneralPoolBase *pool_) {
    WriteLock w_lock(mutex);

    // insert a pool into the map
    pools.insert(GeneralPoolMap::value_type(pool_->getId(), pool_));

    return;
}

void PoolManager::unregisterPool(GeneralPoolBase *pool_) {
    WriteLock w_lock(mutex);

    // find an element and remove it from the map
    GeneralPoolMap::iterator fit = pools.find(pool_->getId());
    if (fit != pools.end()) {
        pools.erase(fit);
    }

    return;
}

void PoolManager::dump() {
    // lock the map
    ReadLock r_lock(mutex);

    // manage pools
    for (GeneralPoolMap::iterator it = pools.begin(); it != pools.end(); it++) {
        (*it).second->dump();
    }

    return;
}

#ifdef _MEM_STATS_
char *PoolManager::printDateTime(const time_t &dtm) {
    char *dt = ctime(&dtm);
    *(dt + strlen(dt) - 1) = '\0';
    return dt;
}
#endif
} // namespace DaqDB
