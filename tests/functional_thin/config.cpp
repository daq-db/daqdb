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

const unsigned int DEFAULT_PORT = 10002;
const size_t DEFAULT_MSG_MAX_SIZE = 1000000;
const size_t DEFAULT_SCCB_POOL_INTERVAL = 100;
const unsigned int DEFAULT_INSTANT_SWAP = 0;

typedef char DEFAULT_KeyType[16];

void initKvsOptions(DaqDB::Options &options, const std::string &configFile) {
    options.runtime.logFunc = [](std::string msg) {
        BOOST_LOG_SEV(lg::get(), bt::debug) << msg << std::flush;
    };

    /* Set default values */
    options.dht.id = 0;
    options.dht.port = DEFAULT_PORT;
    options.dht.msgMaxsize = DEFAULT_MSG_MAX_SIZE;
    options.dht.sccbPoolInterval = DEFAULT_SCCB_POOL_INTERVAL;
    options.dht.instantSwap = DEFAULT_INSTANT_SWAP;
    options.key.field(0, sizeof(DEFAULT_KeyType));

    if (boost::filesystem::exists(configFile)) {
        std::stringstream errorMsg;
        if (!DaqDB::readConfiguration(configFile, options, errorMsg)) {
            BOOST_LOG_SEV(lg::get(), bt::error) << errorMsg.str();
        }
    }
}
