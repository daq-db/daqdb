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

#include <Const.h>
#include <ZhtClient.h>

#include "tests.h"
#include <config.h>
#include <debug.h>

using namespace std;
using namespace DaqDB;

using zht_const = iit::datasys::zht::dm::Const;

void prepareZhtTests(const string &confFile, const string &neighborsFile) {
    if (!boost::filesystem::exists(confFile)) {
        ofstream confOut(confFile, ios::out);
        confOut << "PROTOCOL TCP" << endl;
        confOut << "PORT 10001" << endl;
        confOut << "MSG_MAXSIZE 1000000" << endl;
        confOut << "SCCB_POLL_INTERVAL 100" << endl;
        confOut << "INSTANT_SWAP 0" << endl;
        confOut.close();
        LOG_INFO << "ZHT config file created";
    }
    if (!boost::filesystem::exists(neighborsFile)) {
        ofstream neighbourOut(neighborsFile, ios::out);
        neighbourOut << "localhost 10001" << endl;
        neighbourOut.close();
        LOG_INFO << "ZHT neighbour file created";
    }
}

void cleanupZhtTests(const string &confFile, const string &neighborsFile) {
    if (boost::filesystem::exists(confFile)) {
        boost::filesystem::remove(confFile);
        LOG_INFO << "ZHT config file removed";
    }
    if (boost::filesystem::exists(neighborsFile)) {
        boost::filesystem::remove(neighborsFile);
        LOG_INFO << "ZHT neighbors file removed";
    }
}

bool testZhtConnect(KVStoreBase *kvs) {
    bool result = true;
    ZHTClient zc;

    const string zhtConf = "zht-ft.conf";
    const string neighborConf = "neighbor-ft.conf";

    zc.init(zhtConf, neighborConf);

    string rsStr;
    int rc = zc.ping();

    if (rc == zht_const::toInt(zht_const::ZSC_REC_SUCC)) {
        LOG_INFO << "Ping: OK";
    } else {
        LOG_INFO << "Error: cannot communicate with server";
        result = false;
    }

    zc.teardown();

    return result;
}
