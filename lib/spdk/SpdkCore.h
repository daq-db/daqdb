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
            return (spBdev->state == SpdkBdevState::SPDK_BDEV_READY);
        else
            return false;
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
