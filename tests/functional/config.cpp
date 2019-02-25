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

#include <boost/filesystem.hpp>

#include "config.h"

const std::string DEFAULT_PMEM_POOL_PATH = "/mnt/pmem/pool.pm";
const size_t DEFAULT_PMEM_POOL_SIZE = 8ull * 1024 * 1024 * 1024;
const size_t DEFAULT_PMEM_ALLOC_UNIT_SIZE = 8;

const size_t DEFAULT_OFFLOAD_ALLOC_UNIT_SIZE = 16384;

bool initKvsOptions(DaqDB::Options &options, const std::string &configFile) {
    options.runtime.logFunc = [](std::string msg) {
        BOOST_LOG_SEV(lg::get(), bt::debug) << msg << std::flush;
    };

    /* Set default values */
    options.dht.id = 0;

    options.pmem.poolPath = DEFAULT_PMEM_POOL_PATH;
    options.pmem.totalSize = DEFAULT_PMEM_POOL_SIZE;
    options.pmem.allocUnitSize = DEFAULT_PMEM_ALLOC_UNIT_SIZE;

    options.key.field(0, sizeof(FuncTestKey::runId));
    options.key.field(1, sizeof(FuncTestKey::subdetectorId));
    options.key.field(2, sizeof(FuncTestKey::eventId), true);

    options.offload.allocUnitSize = DEFAULT_OFFLOAD_ALLOC_UNIT_SIZE;

    if (boost::filesystem::exists(configFile)) {
        std::stringstream errorMsg;
        if (!DaqDB::readConfiguration(configFile, options, errorMsg)) {
            BOOST_LOG_SEV(lg::get(), bt::error) << errorMsg.str();
            return false;
        }
    } else {
        return false;
    }
    return true;
}
