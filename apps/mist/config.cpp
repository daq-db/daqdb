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

#include "config.h"
#include <boost/filesystem.hpp>
#include <iostream>

typedef char DEFAULT_KeyType[16];

void initKvsOptions(DaqDB::Options &options, const std::string &configFile) {
    options.runtime.logFunc = [](std::string msg) {
        std::cout << msg << std::endl;
    };

    /* Set default values */
    options.dht.id = 0;

    options.key.field(0, sizeof(DEFAULT_KeyType));

    if (boost::filesystem::exists(configFile)) {
        std::stringstream errorMsg;
        if (!DaqDB::readConfiguration(configFile, options, errorMsg)) {
            std::cout << errorMsg.str() << std::flush;
        }
    }
}
