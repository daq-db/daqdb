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

#include <daqdb/Options.h>

#include "debug.h"
#include <config/Configuration.h>

struct __attribute__((packed)) FuncTestKey {
    FuncTestKey() : eventId(0), subdetectorId(0), runId(0) {};
    FuncTestKey(uint64_t e, uint16_t s, uint16_t r)
        : eventId(e), subdetectorId(s), runId(r) {}
    uint16_t runId;
    uint16_t subdetectorId;
    uint64_t eventId;
};

bool initKvsOptions(DaqDB::Options &options, const std::string &configFile);
