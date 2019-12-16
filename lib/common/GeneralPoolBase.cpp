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
#include <string.h>

#include "GeneralPoolBase.h"
#include "PoolManager.h"

namespace MemMgmt {
unsigned short GeneralPoolBase::idGener = 1;

GeneralPoolBase::GeneralPoolBase(unsigned short id_, const char *name_)
    : stackTraceFile(0), userId(id_) {
    // set the id
    {
        WriteLock w_lock(generMutex);
        id = GeneralPoolBase::idGener++;
    }

    // set the name
    if (name_) {
        strncpy(name, name_, sizeof(name) - 1);
        name[sizeof(name) - 1] = '\0';
    } else {
        name[0] = '\0';
    }

    // initialize rttiName
    rttiName[0] = '\0';

    // register with the PoolManager
    PoolManager &pm = PoolManager::getInstance();
    pm.registerPool(this);
}

GeneralPoolBase::~GeneralPoolBase() {
    // unregister with the PoolManager
    PoolManager &pm = PoolManager::getInstance();
    pm.unregisterPool(this);

    // close the traceStatsFile
    if (stackTraceFile) {
        fflush(stackTraceFile);
        fclose(stackTraceFile);
    }
}

void GeneralPoolBase::setRttiName(const char *_rttiName) {
    strncpy(rttiName, _rttiName, sizeof(rttiName) - 1);
    rttiName[sizeof(rttiName) - 1] = '\0';
}
} // namespace MemMgmt
