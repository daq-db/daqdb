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
 * mq_proxy_stub.cpp
 *
 *  Created on: Jun 21, 2013
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

#include "mq_proxy_stub.h"

MQProxy::MQProxy() : _mc(1) {}

MQProxy::MQProxy(const unsigned int key0) : _mc(key0) {}

MQProxy::~MQProxy() {}

bool MQProxy::send(const void *sendbuf, const size_t sendcount) {

    return _mc.xmit(sendbuf, sendcount);
}

bool MQProxy::recv(void *recvbuf, size_t &recvcount) {

    return _mc.recv(recvbuf, recvcount);
}

bool MQProxy::sendrecv(const void *sendbuf, const size_t sendcount,
                       void *recvbuf, size_t &recvcount) {

    bool sent_bool = _mc.xmit(sendbuf, sendcount);

    bool recv_bool = _mc.recv(recvbuf, recvcount);

    return sent_bool && recv_bool;
}

MQStub::MQStub() : _ms(1) {}

MQStub::MQStub(const unsigned int key0) : _ms(key0) {}

MQStub::~MQStub() {}

bool MQStub::send(const void *sendbuf, const size_t sendcount) {

    return _ms.xmit(sendbuf, sendcount);
}

bool MQStub::recv(void *recvbuf, size_t &recvcount) {

    return _ms.recv(recvbuf, recvcount);
}

bool MQStub::teardown() { return _ms.destroy(); }
