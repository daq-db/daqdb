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

#include <string>

#include <daqdb/Options.h>

#include "SpdkBdev.h"
#include "SpdkThread.h"

namespace DaqDB {

const std::string SPDK_APP_ENV_NAME = "DaqDB";
const std::string DEFAULT_SPDK_CONF_FILE = "spdk.conf";

enum class SpdkState : std::uint8_t {
    SPDK_INIT = 0,
    SPDK_READY,
    SPDK_ERROR,
    SPDK_STOPPED
};

class SpdkCore {
  public:
    SpdkCore(OffloadOptions offloadOptions);

    /**
     * SPDK requires passing configuration through file.
     * This function creates temporary configuration file
     * in format supported by SPDK.
     *
     * @return false when cannot create the file or there are no NVMe devices in
     * _offloadOptions, true otherwise
     */
    bool createConfFile(void);

    /**
     * This functions first checks if configuration file is correct and attach
     * it as default configuration to SPDK environment
     *
     * @return false when cannot allocate required structures or file format is
     * not correct, true otherwise
     */
    bool attachConfigFile(void);

    bool spdkEnvInit(void);

    void spdkBdevModuleInit(void);

    void removeConfFile(void);

    inline bool isOffloadEnabled() {
        if (state == SpdkState::SPDK_READY)
            return (spBdev->spBdevCtx->state == SPDK_BDEV_READY);
        else
            return false;
    }

    SpdkBdev *getBdev(void) {
        return spBdev.get();
    }

    bool isSpdkReady() {
        return state == SpdkState::SPDK_READY ? true : false;
    }

    std::atomic<SpdkState> state;

    /**
     * This thread is used during initialization and later for polling SPDK
     * completion queue.
     */
    std::unique_ptr<SpdkThread> spSpdkThread;

    std::unique_ptr<SpdkBdev> spBdev;

    OffloadOptions offloadOptions;

  private:
    inline bool isNvmeInOptions() {
        return !offloadOptions.nvmeAddr.empty() &&
               !offloadOptions.nvmeName.empty();
    }
    inline std::string getNvmeAddress() { return offloadOptions.nvmeAddr; }
    inline std::string getNvmeName() { return offloadOptions.nvmeName; }

    std::string _confFile;
};

} // namespace DaqDB
