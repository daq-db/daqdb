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

#include <libconfig.h++>

#include "debug.h"
#include <config/Configuration.h>
#include <daqdb/Options.h>

struct __attribute__((packed)) FuncTestKey {
    uint8_t eventId[5];
    uint8_t detectorId;
    uint16_t componentId;
};

bool initKvsOptions(DaqDB::Options &options, const std::string &configFile);
