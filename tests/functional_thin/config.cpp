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

bool initKvsOptions(DaqDB::Options &options, const std::string &configFile) {
    options.runtime.logFunc = [](std::string msg) {
        BOOST_LOG_SEV(lg::get(), bt::debug) << msg << std::flush;
    };

    /* Set default values */
    options.dht.id = 0;
    options.key.field(0, sizeof(FuncTestKey::runId));
    options.key.field(1, sizeof(FuncTestKey::subdetectorId));
    options.key.field(2, sizeof(FuncTestKey::eventId), true);

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
