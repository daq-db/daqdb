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
 * ip_proxy_stub.h
 *
 *  Created on: Jun 25, 2013
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

#ifndef IPPROTOCOL_H_
#define IPPROTOCOL_H_

#include "proxy_stub.h"
#include <pthread.h>

#include <map>
#include <string>
using namespace std;

/*
 *
 */
class IPProtoProxy : public ProtoProxy {
  public:
    typedef map<string, pthread_mutex_t> MAP;
    typedef MAP::iterator MIT;

  public:
    IPProtoProxy();
    virtual ~IPProtoProxy();
    virtual bool teardown();

  protected:
    virtual int getSockCached(const string &host, const uint &port) = 0;
    virtual int makeClientSocket(const string &host, const uint &port) = 0;
    virtual int recvFrom(int sock, void *recvbuf) = 0;
    virtual int loopedrecv(int sock, string &srecv) = 0;

    virtual int reuseSock(int sock);

    virtual pthread_mutex_t *getSockMutex(const string &host, const uint &port);
    virtual void putSockMutex(const string &host, const uint &port);

  protected:
    int loopedrecv(int sock, void *senderAddr, string &srecv);

  private:
    static void init_XX_MUTEX();

  protected:
    static pthread_mutex_t CC_MUTEX; // mutex for connection cache
    static pthread_mutex_t MC_MUTEX; // mutex for mutex cache

  private:
    static bool INIT_CC_MUTEX;
    static bool INIT_MC_MUTEX;

  private:
    MAP MUTEX_CACHE;
};

class IPProtoStub : public ProtoStub {
  public:
    IPProtoStub();
    virtual ~IPProtoStub();
};

#endif /* IPPROTOCOL_H_ */
