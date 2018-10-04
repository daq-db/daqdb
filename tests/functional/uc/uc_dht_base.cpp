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

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

#include "uc.h"

#include <Const.h>
#include <ZhtClient.h>
#include <debug.h>

using zht_const = iit::datasys::zht::dm::Const;

void prepare_zht_tests(const std::string &confFile,
                       const std::string &neighborsFile) {
    if (!boost::filesystem::exists(confFile)) {
        std::ofstream confOut(confFile, std::ios::out);
        confOut << "PROTOCOL TCP" << std::endl;
        confOut << "PORT 10001" << std::endl;
        confOut << "MSG_MAXSIZE 1000000" << std::endl;
        confOut << "SCCB_POLL_INTERVAL 100" << std::endl;
        confOut << "INSTANT_SWAP 0" << std::endl;
        confOut.close();
        BOOST_LOG_SEV(lg::get(), bt::info) << "ZHT config file created";
    }
    if (!boost::filesystem::exists(neighborsFile)) {
        std::ofstream neighbourOut(neighborsFile, std::ios::out);
        neighbourOut << "localhost 10001" << std::endl;
        neighbourOut.close();
        BOOST_LOG_SEV(lg::get(), bt::info) << "ZHT neighbour file created";
    }
}

void cleanup_zht_tests(const std::string &confFile,
                       const std::string &neighborsFile) {
    if (boost::filesystem::exists(confFile)) {
        boost::filesystem::remove(confFile);
        BOOST_LOG_SEV(lg::get(), bt::info) << "ZHT config file removed";
    }
    if (boost::filesystem::exists(neighborsFile)) {
        boost::filesystem::remove(neighborsFile);
        BOOST_LOG_SEV(lg::get(), bt::info) << "ZHT neighbors file removed";
    }
}

bool use_case_zht_connect(std::shared_ptr<DaqDB::KVStoreBase> &spKvs,
                          const std::string &confFile,
                          const std::string &neighborsFile) {
    USE_CASE_LOG("use_case_zht_connect");

    bool result = true;
    ZHTClient zc;
    zc.init(confFile, neighborsFile);

    string rsStr;
    int rc = zc.ping();

    if (rc == zht_const::toInt(zht_const::ZSC_REC_SUCC)) {
        BOOST_LOG_SEV(lg::get(), bt::info) << "Ping: OK";
    } else {
        BOOST_LOG_SEV(lg::get(), bt::info)
            << "Error: cannot communicate with server";
        result = false;
    }

    zc.teardown();

    return result;
}
