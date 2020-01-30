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

#include <stdio.h>
#include <time.h>

#include <iostream>
#include <sstream>
#include <string>

#include "BdevStats.h"

namespace DaqDB {

/*
 * BdevStats
 */
std::ostringstream &BdevStats::formatWriteBuf(std::ostringstream &buf,
                                              const char *bdev_addr) {
    buf << "bdev_addr[" << bdev_addr << "] write_compl_cnt[" << write_compl_cnt
        << "] write_err_cnt[" << write_err_cnt << "] outs_io_cnt["
        << outstanding_io_cnt << "]";
    return buf;
}

std::ostringstream &BdevStats::formatReadBuf(std::ostringstream &buf,
                                             const char *bdev_addr) {
    buf << "bdev_addr[" << bdev_addr << "] read_compl_cnt[" << read_compl_cnt
        << "] read_err_cnt[" << read_err_cnt << "] outs_io_cnt["
        << outstanding_io_cnt << "]";
    return buf;
}

void BdevStats::printWritePer(std::ostream &os, const char *bdev_addr) {
    if (!(write_compl_cnt % quant_per)) {
        std::ostringstream buf;
        char time_buf[128];
        time_t now = time(0);
        strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S.000",
                 localtime(&now));
        os << formatWriteBuf(buf, bdev_addr).str() << " " << time_buf
           << std::endl;
    }
}

void BdevStats::printReadPer(std::ostream &os, const char *bdev_addr) {
    if (!(read_compl_cnt % quant_per)) {
        std::ostringstream buf;
        char time_buf[128];
        time_t now = time(0);
        strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S.000",
                 localtime(&now));
        os << formatReadBuf(buf, bdev_addr).str() << " " << time_buf
           << std::endl;
    }
}

} // namespace DaqDB
