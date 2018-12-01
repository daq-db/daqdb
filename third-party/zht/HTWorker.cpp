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
 * HTWorker.cpp
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

#include "HTWorker.h"

#include "ConfHandler.h"
#include "Const-impl.h"
#include "Env.h"

#include <cstring>
#include <iostream>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

using namespace std;
using namespace iit::datasys::zht::dm;
using namespace DaqDB;

WorkerThreadArg::WorkerThreadArg() : _stub(NULL) {}

WorkerThreadArg::WorkerThreadArg(const ZPack &zpack, const ProtoAddr &addr,
                                 const ProtoStub *const stub)
    : _zpack(zpack), _addr(addr), _stub(stub) {}

WorkerThreadArg::~WorkerThreadArg() {}

NoVoHT *HTWorker::PMAP = NULL;

HTWorker::QUEUE *HTWorker::PQUEUE = new QUEUE();

bool HTWorker::FIRST_ASYNC = false;

int HTWorker::SCCB_POLL_INTERVAL = 100; // Env::get_sccb_poll_interval();

HTWorker::HTWorker() : _stub(NULL), _instant_swap(get_instant_swap()) {

    init_me();
}

HTWorker::HTWorker(DaqDB::KVStoreBase *kvs)
    : _stub(NULL), _daqdb(kvs), _instant_swap(get_instant_swap()) {
    init_me();
}

HTWorker::HTWorker(const ProtoAddr &addr, const ProtoStub *const stub)
    : _addr(addr), _stub(stub), _instant_swap(get_instant_swap()) {

    init_me();
}

HTWorker::~HTWorker() {}

string HTWorker::run(const char *buf) {

    string result;

    ZPack zpack;
    string str(buf);
    zpack.ParseFromString(str);

    if (zpack.opcode() == Const::ZSC_OPC_LOOKUP) {

        result = lookup(zpack);
    } else if (zpack.opcode() == Const::ZSC_OPC_INSERT) {

        result = insert(zpack);
    } else if (zpack.opcode() == Const::ZSC_OPC_APPEND) {

        result = append(zpack);
    } else if (zpack.opcode() == Const::ZSC_OPC_CMPSWP) {

        result = compare_swap(zpack);
    } else if (zpack.opcode() == Const::ZSC_OPC_REMOVE) {

        result = remove(zpack);
    } else if (zpack.opcode() == Const::ZSC_OPC_STCHGCB) {

        result = state_change_callback(zpack);
    } else if (zpack.opcode() == Const::ZSC_OPC_PING) {
        result = ping(zpack);
    } else {
        result = Const::ZSC_REC_UOPC;
    }

    return result;
}

char *HTWorker::run(const char *buf, int *resultSize) {

    string resultStr = Const::ZSC_REC_UOPC;
    char *result;

    // @TODO jradtke remove zpack and make all communication thru DaqdbDhtMsg
    ZPack zpack;
    string str(buf);
    zpack.ParseFromString(str);

    if (zpack.opcode() == Const::ZSC_OPC_LOOKUP) {

        resultStr = lookup(zpack);
    } else if (zpack.opcode() == Const::ZSC_OPC_INSERT) {

        resultStr = insert(zpack);
    } else if (zpack.opcode() == Const::ZSC_OPC_APPEND) {

        resultStr = append(zpack);
    } else if (zpack.opcode() == Const::ZSC_OPC_CMPSWP) {

        resultStr = compare_swap(zpack);
    } else if (zpack.opcode() == Const::ZSC_OPC_REMOVE) {

        resultStr = remove(zpack);
    } else if (zpack.opcode() == Const::ZSC_OPC_STCHGCB) {

        resultStr = state_change_callback(zpack);
    } else if (zpack.opcode() == Const::ZSC_OPC_PING) {
        resultStr = ping(zpack);
    } else {
        DaqdbDhtMsg *msg = (DaqdbDhtMsg *)buf;
        if (checkDaqdbSignature(msg->signature)) {
            switch (msg->opcode) {
            case DaqDB::OPC_INSERT:
                return insert(msg, resultSize);
            case DaqDB::OPC_GET:
                return lookup(msg, resultSize);
            }

        } else {
            resultStr = Const::ZSC_REC_UOPC;
        }
    }

    // @TODO jradtke To remove when all communication thru DaqdbDhtMsg
    *resultSize = resultStr.size();
    result = (char *)calloc(1, *resultSize);
    memcpy(result, resultStr.c_str(), *resultSize);

    return result;
}

string HTWorker::insert_shared(const ZPack &zpack) {

    string result;

    if (zpack.key().empty())
        return Const::ZSC_REC_EMPTYKEY; //-1

    string keyStr = zpack.key();
    string valStr = zpack.val();

    DaqDB::Key key = _daqdb->AllocKey();
    std::memset(key.data(), 0, key.size());
    std::memcpy(key.data(), keyStr.c_str(), keyStr.size());

    DaqDB::Value val = _daqdb->Alloc(key, valStr.size() + 1);
    std::memcpy(val.data(), valStr.c_str(), valStr.size());
    val.data()[val.size() - 1] = '\0';

    try {
        _daqdb->Put(std::move(key), std::move(val));
        result = Const::ZSC_REC_SUCC;
    } catch (DaqDB::OperationFailedException &e) {
        result = Const::ZSC_REC_NONEXISTKEY;
    }

    return result;
}

char *HTWorker::insert(const DaqDB::DaqdbDhtMsg *msg, int *resultSize) {

    DaqdbDhtResult *result =
        (DaqdbDhtResult *)calloc(1, sizeof(DaqdbDhtResult));
    result->signature = DAQDB_MSG_SIGNATURE;

    if (msg->keySize == 0) {
        *resultSize = fillDaqdbResult(
            result, Const::toInt(Const::ZSC_REC_EMPTYKEY), nullptr, 0);
        return (char *)result;
    }

    Key key = _daqdb->AllocKey();
    std::memcpy(key.data(), msg->msg, msg->keySize);

    Value val = _daqdb->Alloc(key, msg->valSize);
    std::memcpy(val.data(), getValueFromDaqdbDhtMsg(msg), msg->valSize);

    try {
        _daqdb->Put(std::move(key), std::move(val));
        *resultSize = fillDaqdbResult(result, Const::toInt(Const::ZSC_REC_SUCC),
                                      nullptr, 0);
        return (char *)result;
    } catch (DaqDB::OperationFailedException &e) {
        *resultSize = fillDaqdbResult(
            result, Const::toInt(Const::ZSC_REC_NONEXISTKEY), nullptr, 0);
        return (char *)result;
    }
}

string HTWorker::insert(const ZPack &zpack) {

    string result = insert_shared(zpack);

#ifdef SCCB
    _stub->sendBack(_addr, result.data(), result.size());
    return "";
#else
    return result;
#endif
}

char *HTWorker::lookup(const DaqDB::DaqdbDhtMsg *msg, int *resultSize) {

    DaqdbDhtResult *result;

    if (msg->keySize == 0) {
        result = (DaqdbDhtResult *)calloc(1, sizeof(DaqdbDhtResult));
        *resultSize = fillDaqdbResult(
            result, Const::toInt(Const::ZSC_REC_EMPTYKEY), nullptr, 0);
        return (char *)result;
    }

    Key key = _daqdb->AllocKey();
    std::memcpy(key.data(), msg->msg, msg->keySize);

    try {
        auto val = _daqdb->Get(key);

        result =
            (DaqdbDhtResult *)calloc(1, sizeof(DaqdbDhtResult) + val.size());
        *resultSize = fillDaqdbResult(result, Const::toInt(Const::ZSC_REC_SUCC),
                                      val.data(), val.size());
        return (char *)result;
    } catch (DaqDB::OperationFailedException &e) {
        result = (DaqdbDhtResult *)calloc(1, sizeof(DaqdbDhtResult));
        *resultSize = fillDaqdbResult(
            result, Const::toInt(Const::ZSC_REC_NONEXISTKEY), nullptr, 0);
        return (char *)result;
    }
}

string HTWorker::lookup_shared(const ZPack &zpack) {

    string result;

    result = Const::ZSC_REC_SUCC;

    if (zpack.key().empty())
        return Const::ZSC_REC_EMPTYKEY;
    string keyStr = zpack.key();

    DaqDB::Key key = _daqdb->AllocKey();
    std::memset(key.data(), 0, key.size());
    std::memcpy(key.data(), keyStr.c_str(), keyStr.size());
    try {
        auto val = _daqdb->Get(key);
        result = Const::ZSC_REC_SUCC;
        result.append(val.data());
    } catch (DaqDB::OperationFailedException &e) {
        result = Const::ZSC_REC_NONEXISTKEY;
        result.append("Empty");
    }

    return result;
}

string HTWorker::ping(const ZPack &zpack) { return Const::ZSC_REC_SUCC; }

string HTWorker::lookup(const ZPack &zpack) {

    string result = lookup_shared(zpack);

#ifdef SCCB
    _stub->sendBack(_addr, result.data(), result.size());
    return "";
#else
    return result;
#endif
}

string HTWorker::append_shared(const ZPack &zpack) {

    string result;

    if (zpack.key().empty())
        return Const::ZSC_REC_EMPTYKEY; //-1

    string key = zpack.key();
    int ret = PMAP->append(key, zpack.SerializeAsString());

    if (ret != 0) {

        printf("thread[%lu] DB Error: fail to append, rcode = %d\n",
               pthread_self(), ret);
        fflush(stdout);

        result = Const::ZSC_REC_NONEXISTKEY; //-92
    } else {

        if (_instant_swap) {
            PMAP->writeFileFG();
        }

        result = Const::ZSC_REC_SUCC; // 0, succeed.
    }

    return result;
}

string HTWorker::append(const ZPack &zpack) {

    string result = append_shared(zpack);

#ifdef SCCB
    _stub->sendBack(_addr, result.data(), result.size());
    return "";
#else
    return result;
#endif
}

string HTWorker::state_change_callback(const ZPack &zpack) {

    WorkerThreadArg *wta = new WorkerThreadArg(zpack, _addr, _stub);
    PQUEUE->push(wta); // queue the WorkerThreadArg to be used in thread
                       // function

    if (!FIRST_ASYNC) {
        pthread_t tid;
        pthread_create(&tid, NULL, threaded_state_change_callback, NULL);
        FIRST_ASYNC = true;
    }

    return "";
}

void *HTWorker::threaded_state_change_callback(void *arg) {

    WorkerThreadArg *pwta = NULL;
    while (true) {
        while (PQUEUE->pop(pwta)) { // dequeue the WorkerThreadArg

            string result = state_change_callback_internal(pwta->_zpack);

            int mslapsed = 0;
            int lease = atoi(pwta->_zpack.lease().c_str());

            // printf("poll_interval: %d\n", poll_interval);

            while (result != Const::ZSC_REC_SUCC) {

                mslapsed += SCCB_POLL_INTERVAL;
                usleep(SCCB_POLL_INTERVAL * 1000);

                if (mslapsed >= lease)
                    break;

                result = state_change_callback_internal(pwta->_zpack);
            }

            pwta->_stub->sendBack(pwta->_addr, result.data(), result.size());

            /*pwta->_htw->_stub->sendBack(pwta->_htw->_addr, result.data(),
             result.size());*/

            delete pwta;
            pwta = NULL;
        }
    }
}

string HTWorker::state_change_callback_internal(const ZPack &zpack) {

    string result;

    if (zpack.key().empty())
        return Const::ZSC_REC_EMPTYKEY; //-1

    string key = zpack.key();
    string *ret = PMAP->get(key);

    if (ret == NULL) {

        printf("thread[%lu] DB Error: lookup found nothing\n", pthread_self());
        fflush(stdout);

        result = Const::ZSC_REC_NONEXISTKEY;
    } else {

        ZPack rltpack;
        rltpack.ParseFromString(*ret);

        if (zpack.val() == rltpack.val()) {

            result = Const::ZSC_REC_SUCC; // 0, succeed.
        } else {

            result = Const::ZSC_REC_SCCBPOLLTRY;
        }
    }

    return result;
}

string HTWorker::compare_swap(const ZPack &zpack) {

    if (zpack.key().empty())
        return Const::ZSC_REC_EMPTYKEY; //-1

    string result = compare_swap_internal(zpack);

    string lkpresult = lookup_shared(zpack);

    result.append(erase_status_code(lkpresult));

#ifdef SCCB
    _stub->sendBack(_addr, result.data(), result.size());
    return "";
#else
    return result;
#endif
}

string HTWorker::compare_swap_internal(const ZPack &zpack) {

    string ret;

    /*get Package stored by lookup*/
    string lresult = lookup_shared(zpack);
    ZPack lzpack;
    lresult = erase_status_code(lresult);
    lzpack.ParseFromString(lresult);

    /*get seen_value passed in*/
    string seen_value_passed_in = zpack.val();

    /*get seen_value stored*/
    string seen_value_stored = lzpack.val();

    /*	printf("{%s}:{%s,%s}\n", zpack.key().c_str(), zpack.val().c_str(),
     zpack.newval().c_str());*/

    /*they are equivalent, compare and swap*/
    if (!seen_value_passed_in.compare(seen_value_stored)) {

        lzpack.set_val(zpack.newval());

        return insert_shared(lzpack);

    } else {

        return Const::ZSC_REC_SRVEXP;
    }
}

string HTWorker::remove_shared(const ZPack &zpack) {

    string result;

    result = Const::ZSC_REC_SUCC;

    if (zpack.key().empty())
        return Const::ZSC_REC_EMPTYKEY;

    string keyStr = zpack.key();

    DaqDB::Key key = _daqdb->AllocKey();
    std::memset(key.data(), 0, key.size());
    std::memcpy(key.data(), keyStr.c_str(), keyStr.size());
    try {
        _daqdb->Remove(key);
        result = Const::ZSC_REC_SUCC;
    } catch (DaqDB::OperationFailedException &e) {
        result = Const::ZSC_REC_NONEXISTKEY;
    }

    return result;
}

string HTWorker::remove(const ZPack &zpack) {

    string result = remove_shared(zpack);

#ifdef SCCB
    _stub->sendBack(_addr, result.data(), result.size());
    return "";
#else
    return result;
#endif
}

string HTWorker::erase_status_code(string &val) { return val.substr(3); }

string HTWorker::get_novoht_file() { return ConfHandler::NOVOHT_FILE; }

void HTWorker::init_me() {

    if (PMAP == NULL)
        PMAP = new NoVoHT(get_novoht_file(), 100000, 10000, 0.7);
}

bool HTWorker::get_instant_swap() {

    string swap = ConfHandler::get_zhtconf_parameter(Const::INSTANT_SWAP);

    int flag = atoi(swap.c_str());

    bool result;

    if (flag == 1)
        result = true;
    else if (flag == 0)
        result = false;
    else
        result = false;

    return result;
}
