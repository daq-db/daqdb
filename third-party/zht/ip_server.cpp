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
 * ip_server.cpp
 *
 *  Created on: Jul 5, 2013
 *      Author: Xiaobingo
 *      Contributor: Tony, KWang, DZhao
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

#include "ip_server.h"

#include "ProxyStubFactory.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

using namespace std;

IPServer::IPServer() : _stub(ProxyStubFactory::createStub()) {}

IPServer::IPServer(int hash_mask,
                   std::map<std::pair<int, int>, int> &rangeToHost)
    : _stub(ProxyStubFactory::createStub(hash_mask, rangeToHost)) {}

IPServer::~IPServer() {

    if (_stub != NULL) {

        delete _stub;
        _stub = NULL;
    }
}

void IPServer::process(const int &fd, const char *const buf, sockaddr sender) {

    if (_stub == 0) {

        fprintf(
            stderr,
            "IPServer::process(): error on ProxyStubFactory::createStub().\n");
        return;
    }

    ProtoAddr pa;
    pa.fd = fd;
    pa.sender = calloc(1, sizeof(sockaddr));
    memcpy(pa.sender, &sender, sizeof(sockaddr));

    string bufstr(buf);
    _stub->recvsend(pa, bufstr.c_str());
}
