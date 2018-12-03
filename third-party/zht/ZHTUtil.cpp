/*
 * Copyright 2010-2020 DatasysLab@iit.edu(http://datasys.cs.iit.edu/index.html)
 *      Director: Ioan Raicu(iraicu@cs.iit.edu)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * This file is part of ZHT
 * library(http://datasys.cs.iit.edu/projects/ZHT/index.html). Tonglin
 * Li(tli13@hawk.iit.edu) with nickname Tony, Xiaobing
 * Zhou(xzhou40@hawk.iit.edu) with nickname Xiaobingo, Ke
 * Wang(kwang22@hawk.iit.edu) with nickname KWang, Dongfang
 * Zhao(dzhao8@@hawk.iit.edu) with nickname DZhao, Ioan
 * Raicu(iraicu@cs.iit.edu).
 *
 * ZHTUtil.cpp
 *
 *  Created on: Jun 25, 2013
 *      Author: Tony
 *      Contributor: Xiaobingo, KWang, DZhao
 */

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

#include "ZHTUtil.h"

#include "ConfHandler.h"
#include "Util.h"

#include <algorithm>
#include <arpa/inet.h>
#include <netdb.h>

#include "zpack.pb.h"

#include "Daqdb.h"

using namespace iit::datasys::zht::dm;
using namespace DaqDB;

ZHTUtil::ZHTUtil() {}

ZHTUtil::~ZHTUtil() {}

HostEntity ZHTUtil::getHostEntityByKey(const char *msg, size_t msgSize,
                                       int hash_mask,
                                       RangeToHost *rangeToHost) {

    DaqdbDhtMsg *daqdbMsg = (DaqdbDhtMsg *)msg;
    string key(daqdbMsg->msg, daqdbMsg->keySize);

    uint64_t hashcode = HashUtil::genHash(key, hash_mask, rangeToHost);
    size_t node_size = ConfHandler::NeighborVector.size();

    int index =
        0; // if not found then should be send to first on neighbors list
    if (rangeToHost) {
        for (auto entry : *rangeToHost) {
            auto start = entry.first.first;
            auto end = entry.first.second;
            if ((start <= hashcode) && (end >= hashcode)) {
                index = entry.second;
                break;
            }
        }
    }

    ConfEntry ce = ConfHandler::NeighborVector.at(index);

    return buildHostEntity(ce.name(), atoi(ce.value().c_str()));
}

HostEntity ZHTUtil::getHostEntityByKey(const string &msg, int hash_mask,
                                       RangeToHost *rangeToHost) {

    ZPack zpack;
    zpack.ParseFromString(msg); // to debug

    auto key = zpack.key();
    uint64_t hashcode = HashUtil::genHash(zpack.key(), hash_mask, rangeToHost);
    size_t node_size = ConfHandler::NeighborVector.size();

    int index =
        0; // if not found then should be send to first on neighbors list
    if (rangeToHost) {
        for (auto entry : *rangeToHost) {
            auto start = entry.first.first;
            auto end = entry.first.second;
            if ((start <= hashcode) && (end >= hashcode)) {
                index = entry.second;
                break;
            }
        }
    }

    ConfEntry ce = ConfHandler::NeighborVector.at(index);

    return buildHostEntity(ce.name(), atoi(ce.value().c_str()));
}

HostEntity ZHTUtil::getHostEntityByKey(const string &msg) {

    ZPack zpack;
    zpack.ParseFromString(msg); // to debug

    auto key = zpack.key();
    uint64_t hascode = HashUtil::genHash(key);
    size_t node_size = ConfHandler::NeighborVector.size();
    int index = hascode % node_size;

    ConfEntry ce = ConfHandler::NeighborVector.at(index);

    return buildHostEntity(ce.name(), atoi(ce.value().c_str()));
}

HostEntity ZHTUtil::buildHostEntity(const string &host, const uint &port) {

    HostEntity he;

    /*
     struct sockaddr_in si_other;
     hostent *record;
     in_addr *address;
     string ip_address;

     record = gethostbyname(host.c_str());
     address = (in_addr *) record->h_addr;
     ip_address = inet_ntoa(*address);

     memset((char *) &si_other, 0, sizeof(si_other));
     si_other.sin_family = AF_INET;
     si_other.sin_port = htons(port);
     if (inet_aton(ip_address.c_str(), &si_other.sin_addr) == 0) {
     fprintf(stderr, "inet_aton() failed\n");
     }

     he.si = si_other;
     he.host = host;
     he.port = port;
     he.valid = true;
     he.sock = -1;*/

    he.host = host;
    he.port = port;
    he.sock = -1;

    return he;
}

const uint IdHelper::ID_LEN = 20;

IdHelper::IdHelper() {}

IdHelper::~IdHelper() {}

uint64_t IdHelper::genId() {

    return HashUtil::genHash(HashUtil::randomString(62).c_str());
}
