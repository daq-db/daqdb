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
 * ProxyStubFactory.cpp
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

#include "ProxyStubFactory.h"

#ifdef PF_INET
#include "mq_proxy_stub.h"
#include "tcp_proxy_stub.h"
#include "udp_proxy_stub.h"
#elif MPI_INET
#include "mpi_proxy_stub.h"
#include "mq_proxy_stub.h"
#endif

#include "ConfHandler.h"
using namespace iit::datasys::zht::dm;

ProxyStubFactory::ProxyStubFactory() {}

ProxyStubFactory::~ProxyStubFactory() {}

ProtoProxy *ProxyStubFactory::createProxy() {

    ConfHandler::MAP *zpmap = &ConfHandler::ZHTParameters;

    ConfEntry ce_tcp(Const::PROTO_NAME, Const::PROTO_VAL_TCP); // TCP
    ConfEntry ce_udp(Const::PROTO_NAME, Const::PROTO_VAL_UDP); // UDP
    ConfEntry ce_mpi(Const::PROTO_NAME, Const::PROTO_VAL_MPI); // MPI

#ifdef PF_INET

    if (zpmap->find(ce_tcp.toString()) != zpmap->end())
        return new TCPProxy();

    if (zpmap->find(ce_udp.toString()) != zpmap->end())
        return new UDPProxy();

    if (zpmap->find(ce_mpi.toString()) != zpmap->end())
        return new MQProxy();
#endif

    return 0;
}

ProtoProxy *
ProxyStubFactory::createProxy(int hash_mask,
                              std::map<std::pair<int, int>, int> *rangeToHost) {

    ConfHandler::MAP *zpmap = &ConfHandler::ZHTParameters;

    ConfEntry ce_tcp(Const::PROTO_NAME, Const::PROTO_VAL_TCP); // TCP
    ConfEntry ce_udp(Const::PROTO_NAME, Const::PROTO_VAL_UDP); // UDP
    ConfEntry ce_mpi(Const::PROTO_NAME, Const::PROTO_VAL_MPI); // MPI

#ifdef PF_INET

    if (zpmap->find(ce_tcp.toString()) != zpmap->end())
        return new TCPProxy(hash_mask, rangeToHost);

    if (zpmap->find(ce_udp.toString()) != zpmap->end())
        return new UDPProxy();

    if (zpmap->find(ce_mpi.toString()) != zpmap->end())
        return new MQProxy();
#endif

    return 0;
}

ProtoStub *ProxyStubFactory::createStub() {

    ConfHandler::MAP *zpmap = &ConfHandler::ZHTParameters;

    ConfEntry ce_tcp(Const::PROTO_NAME, Const::PROTO_VAL_TCP); // TCP
    ConfEntry ce_udp(Const::PROTO_NAME, Const::PROTO_VAL_UDP); // UDP
    ConfEntry ce_mpi(Const::PROTO_NAME, Const::PROTO_VAL_MPI); // MPI

#ifdef PF_INET

    // if (zpmap->find(ce_tcp.toString()) != zpmap->end())
    return new TCPStub();

    if (zpmap->find(ce_udp.toString()) != zpmap->end())
        return new UDPStub();
#elif MPI_INET

    if (zpmap->find(ce_mpi.toString()) != zpmap->end())
        return new MPIStub();
#endif

    return 0;
}

ProtoStub *
ProxyStubFactory::createStub(int hash_mask,
                             std::map<std::pair<int, int>, int> *rangeToHost,
                             DaqDB::KVStoreBase *kvs) {

    ConfHandler::MAP *zpmap = &ConfHandler::ZHTParameters;

    ConfEntry ce_tcp(Const::PROTO_NAME, Const::PROTO_VAL_TCP); // TCP
    ConfEntry ce_udp(Const::PROTO_NAME, Const::PROTO_VAL_UDP); // UDP
    ConfEntry ce_mpi(Const::PROTO_NAME, Const::PROTO_VAL_MPI); // MPI

#ifdef PF_INET

    if (zpmap->find(ce_tcp.toString()) != zpmap->end())
        return new TCPStub(hash_mask, rangeToHost, kvs);

    if (zpmap->find(ce_udp.toString()) != zpmap->end())
        return new UDPStub();
#elif MPI_INET

    if (zpmap->find(ce_mpi.toString()) != zpmap->end())
        return new MPIStub();
#endif

    return 0;
}
