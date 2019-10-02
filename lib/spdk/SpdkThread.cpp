/**
 *  Copyright (c) 2019 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License. 
 */

#include <iostream>
#include <string>
#include <thread>

#include "spdk/stdinc.h"
#include "spdk/cpuset.h"
#include "spdk/queue.h"
#include "spdk/log.h"
#include "spdk/thread.h"
#include "spdk/event.h"
#include "spdk/ftl.h"
#include "spdk/conf.h"
#include "spdk/env.h"
#include "spdk/util.h"

#include "SpdkThread.h"
#include "SpdkBdev.h"
#include <Logger.h>

using namespace std;

namespace DaqDB {

const int SPDK_THREAD_RING_SIZE = 4096;
const string SPDK_THREAD_NAME = "DaqDB";
const unsigned int SPDK_POLLING_TIMEOUT = 1000000UL;

#define SPDK_FIO_POLLING_TIMEOUT 1000000UL

SpdkThread::SpdkThread(SpdkBdev *bdev)
    : spBdev(bdev) {
    spCtx.reset(new SpdkThreadCtx());
}

SpdkThread::~SpdkThread() {
    deinit();
}

bool SpdkThread::init() {
    //_thread = new std::thread(&SpdkThread::threadStart, this);
    //sleep(5);
    return true;
}

void SpdkThread::deinit() {
    spdk_app_stop(0);
    //if ( _thread ) {
        //_thread->join();
    //}
}

//void SpdkThread::threadStart() {
    //spBdev->init();
//}

} // namespace DaqDB
