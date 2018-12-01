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
 * HTWorker.h
 *
 *  Created on: Sep 10, 2012
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

#ifndef HTWORKER_H_
#define HTWORKER_H_

#include "TSafeQueue-impl.h"
#include "novoht.h"
#include "proxy_stub.h"
#include "zpack.pb.h"
#include <queue>
#include <string>

#include "Daqdb.h"

using namespace std;
using namespace iit::cs550::finalproj;

class HTWorker;

class WorkerThreadArg {

  public:
    WorkerThreadArg();
    WorkerThreadArg(const ZPack &zpack, const ProtoAddr &addr,
                    const ProtoStub *const stub);
    virtual ~WorkerThreadArg();

    ZPack _zpack;
    ProtoAddr _addr;
    const ProtoStub *_stub;
};
/*
 *
 */
class HTWorker {
  public:
    typedef TSafeQueue<WorkerThreadArg *> QUEUE;

  public:
    HTWorker();
    HTWorker(const ProtoAddr &addr, const ProtoStub *const stub);
    HTWorker(DaqDB::KVStoreBase *kvs);
    virtual ~HTWorker();

  public:
    char *run(const char *buf, int *resultSize);
    string run(const char *buf);

  private:
    string ping(const ZPack &zpack);
    string insert(const ZPack &zpack);
    char *insert(const DaqDB::DaqdbDhtMsg *msg, int *resultSize);
    string lookup(const ZPack &zpack);
    char *lookup(const DaqDB::DaqdbDhtMsg *msg, int *resultSize);

    string append(const ZPack &zpack);
    string remove(const ZPack &zpack);
    string compare_swap(const ZPack &zpack);
    string state_change_callback(const ZPack &zpack);

    string insert_shared(const ZPack &zpack);
    string lookup_shared(const ZPack &zpack);
    string append_shared(const ZPack &zpack);
    string remove_shared(const ZPack &zpack);

  private:
    static void *threaded_state_change_callback(void *arg);
    static string state_change_callback_internal(const ZPack &zpack);

  private:
    string compare_swap_internal(const ZPack &zpack);

  private:
    string erase_status_code(string &val);
    string get_novoht_file();
    void init_me();
    bool get_instant_swap();

  private:
    ProtoAddr _addr;
    const ProtoStub *const _stub;
    bool _instant_swap;

  private:
    DaqDB::KVStoreBase *_daqdb;
    static NoVoHT *PMAP;
    static QUEUE *PQUEUE;
    static bool FIRST_ASYNC;
    static int SCCB_POLL_INTERVAL;
};

#endif /* HTWORKER_H_ */
