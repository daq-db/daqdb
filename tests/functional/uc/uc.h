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

#include <daqdb/KVStoreBase.h>

#define USE_CASE_LOG(name)                                                     \
    BOOST_LOG_SEV(lg::get(), bt::info) << std::endl                            \
                                       << std::string(80, '-') << std::endl    \
                                       << name << std::endl                    \
                                       << std::string(80, '-') << std::flush;

bool use_case_sync_base(std::shared_ptr<DaqDB::KVStoreBase> &spKvs);
bool use_case_async_base(std::shared_ptr<DaqDB::KVStoreBase> &spKvs);
bool use_case_sync_offload(std::shared_ptr<DaqDB::KVStoreBase> &spKvs);
