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
 * tcp_proxy_stub.h
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

#ifndef TCP_PROXY_STUB_H_
#define TCP_PROXY_STUB_H_

#include "HTWorker.h"
#include "ip_proxy_stub.h"
#include <daqdb/KVStoreBase.h>
#include <pthread.h>

#include "Daqdb.h"

using namespace std;

/*
 *
 */
class TCPProxy : public IPProtoProxy {
  public:
    typedef map<string, int> MAP;
    typedef MAP::iterator MIT;

  public:
    TCPProxy();
    TCPProxy(int hash_mask, RangeToHost *rangeToHost);
    virtual ~TCPProxy();

    virtual bool sendrecv(const void *sendbuf, const size_t sendcount,
                          void *recvbuf, size_t &recvcount);
    virtual bool teardown();

  protected:
    virtual int getSockCached(const string &host, const uint &port);
    virtual int makeClientSocket(const string &host, const uint &port);
    virtual int recvFrom(int sock, void *recvbuf);
    virtual int loopedrecv(int sock, string &srecv);

  private:
    int sendTo(int sock, const void *sendbuf, int sendcount);

  private:
    // static MAP CONN_CACHE;
    MAP CONN_CACHE;
    // @TODO create separate class for DAQDB customizations
    RangeToHost *_rangeToHost = nullptr;
    int _hash_mask = 0;
};

class TCPStub : public IPProtoStub {
  public:
    TCPStub();
    TCPStub(int hash_mask, RangeToHost *rangeToHost, DaqDB::KVStoreBase *kvs);
    virtual ~TCPStub();

    virtual bool recvsend(ProtoAddr addr, const void *recvbuf);

  public:
    virtual int sendBack(ProtoAddr addr, const void *sendbuf,
                         int sendcount) const;
    DaqDB::KVStoreBase *_daqdb;
};

#endif /* TCP_PROXY_STUB_H_ */
