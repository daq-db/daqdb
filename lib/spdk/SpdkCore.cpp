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

#include "spdk/conf.h"
#include "spdk/env.h"

#include "SpdkCore.h"
#include "SpdkThread.h"

#include <Logger.h>

using namespace std;
namespace bf = boost::filesystem;

namespace DaqDB {

const char *SpdkCore::spdkHugepageDirname = "/mnt/huge_1GB";

SpdkCore::SpdkCore(OffloadOptions offloadOptions)
    : state(SpdkState::SPDK_INIT), offloadOptions(offloadOptions) {
    bool doDevInit = createConfFile();

    spBdev.reset(new SpdkBdev());

    bool rc = spdkEnvInit();
    removeConfFile();

    if (!rc) {
        DAQ_DEBUG("Cannot initialize SPDK environment");
        state = SpdkState::SPDK_ERROR;
        spBdev->state = SpdkBdevState::SPDK_BDEV_ERROR;
        return;
    }

    state = SpdkState::SPDK_READY;
    spSpdkThread.reset(new SpdkThread(spBdev.get()));
    spSpdkThread->init();
}

static void spdkDoneCb(void *cb_arg, int rc) {
    *reinterpret_cast<bool *>(cb_arg) = true;
}

bool SpdkCore::createConfFile(void) {
    if (!bf::exists(DEFAULT_SPDK_CONF_FILE)) {
        if (isNvmeInOptions()) {
            ofstream spdkConf(DEFAULT_SPDK_CONF_FILE, ios::out);
            if (spdkConf) {
                spdkConf << "[Nvme]" << endl
                         << "  TransportID \"trtype:PCIe traddr:"
                         << getNvmeAddress() << "\" " << getNvmeName() << endl;

                spdkConf.close();

                DAQ_DEBUG("SPDK configuration file created");
                return true;
            } else {
                DAQ_DEBUG("Cannot create SPDK configuration file");
                return false;
            }
        } else {
            DAQ_DEBUG(
                "SPDK configuration file creation skipped - no NVMe device");
            return false;
        }
    } else {
        DAQ_DEBUG("SPDK configuration file creation skipped");
        return true;
    }
}

void SpdkCore::removeConfFile(void) {
    if (bf::exists(DEFAULT_SPDK_CONF_FILE)) {
        bf::remove(DEFAULT_SPDK_CONF_FILE);
    }
}

} // namespace DaqDB
