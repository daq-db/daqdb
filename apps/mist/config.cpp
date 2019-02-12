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
