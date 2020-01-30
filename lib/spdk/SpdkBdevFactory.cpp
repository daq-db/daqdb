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

#include <string>

#include <Rqst.h>

#include "SpdkBdev.h"
#include "SpdkBdevFactory.h"
#include "SpdkConf.h"
#include "SpdkDevice.h"
#include "SpdkJBODBdev.h"
#include "SpdkRAID0Bdev.h"
#include <RTree.h>

namespace DaqDB {

SpdkDevice *SpdkBdevFactory::getBdev(SpdkDeviceClass typ) {
    switch (typ) {
    case SpdkDeviceClass::BDEV:
        return new SpdkBdev;
        break; // never reached
    case SpdkDeviceClass::JBOD:
        return new SpdkJBODBdev;
        break; // never reached
    case SpdkDeviceClass::RAID0:
        return new SpdkRAID0Bdev;
        break; // never reached
    }
    return 0;
}

} // namespace DaqDB
