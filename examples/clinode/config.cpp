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

#include <boost/filesystem.hpp>

#include "config.h"

const std::string DEFAULT_PMEM_POOL_PATH = "/mnt/pmem/pool.pm";
const size_t DEFAULT_PMEM_POOL_SIZE = 8ull * 1024 * 1024 * 1024;
const size_t DEFAULT_PMEM_ALLOC_UNIT_SIZE = 1024;
typedef char DEFAULT_KeyType[16];

const size_t DEFAULT_OFFLOAD_ALLOC_UNIT_SIZE = 16384;

void initKvsOptions(DaqDB::Options &options, const std::string &configFile) {
    options.runtime.logFunc = [](std::string msg) {
        BOOST_LOG_SEV(lg::get(), bt::debug) << msg << std::flush;
    };

    /* Set default values */
    options.dht.id = 0;

    options.pmem.poolPath = DEFAULT_PMEM_POOL_PATH;
    options.pmem.totalSize = DEFAULT_PMEM_POOL_SIZE;
    options.pmem.allocUnitSize = DEFAULT_PMEM_ALLOC_UNIT_SIZE;
    options.key.field(0, sizeof(DEFAULT_KeyType));

    options.offload.allocUnitSize = DEFAULT_OFFLOAD_ALLOC_UNIT_SIZE;

    if (boost::filesystem::exists(configFile)) {
        std::stringstream errorMsg;
        if (!DaqDB::readConfiguration(configFile, options, errorMsg)) {
            BOOST_LOG_SEV(lg::get(), bt::error) << errorMsg.str();
        }
    }
}
