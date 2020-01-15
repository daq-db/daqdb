/**
 *  Copyright (c) 2020 Intel Corporation
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

#include <atomic>
#include <iostream>
#include <thread>

#include <math.h>
#include <pthread.h>
#include <unistd.h>

#include "../../../lib/common/BlockingPoller.h"

using namespace std;
using namespace DaqDB;

struct Msg {
    Msg(unsigned int _f1, unsigned long long _f2, unsigned int _msgNo)
        : f1(_f1), f2(_f2), msgNo(_msgNo) {}
    virtual ~Msg() = default;

    unsigned int f1;
    unsigned long long f2;
    unsigned int msgNo;
};

class Consumer : public BlockingPoller<Msg> {
  public:
    Consumer(unsigned int numMsg, useconds_t msgDelay);
    virtual ~Consumer() = default;

    void process() final;
    void mainLoop();

    atomic<int> isRunning;

  protected:
    void _processMsg(Msg *msg);

  private:
    unsigned int _numMsg;
    useconds_t _msgDelay;
};

Consumer::Consumer(unsigned int numMsg, useconds_t msgDelay)
    : BlockingPoller<Msg>(), isRunning(1), _numMsg(numMsg),
      _msgDelay(msgDelay) {}

void Consumer::process() {
    if (requestCount > 0) {
        for (unsigned short RqstIdx = 0; RqstIdx < requestCount; RqstIdx++) {
            Msg *msg = requests[RqstIdx];
            if (msg)
                _processMsg(msg);
            if (_msgDelay)
                usleep(_msgDelay);
        }
        requestCount = 0;
    }
}

void Consumer::mainLoop() {
    pthread_setname_np(pthread_self(), "Consumer");

    while (isRunning) {
        dequeue();
        process();
    }
}

void Consumer::_processMsg(Msg *msg) {
    cout << "Got msg f1[" << msg->f1 << "] f2[" << msg->f2 << "] msgNo["
         << msg->msgNo << "]" << endl
         << flush;
    if (msg->msgNo >= _numMsg)
        isRunning = 3;
}

class Producer {
  public:
    Producer(unsigned int numMsg, useconds_t msgDelay, unsigned int idleCnt);
    virtual ~Producer();

    void mainLoop();

  private:
    Consumer *_consumer;
    thread *_consThread;
    unsigned int _numMsg;
    useconds_t _msgDelay;
    unsigned int _idleCnt;
};

Producer::Producer(unsigned int numMsg, useconds_t msgDelay,
                   unsigned int idleCnt)
    : _consumer(0), _consThread(0), _numMsg(numMsg), _msgDelay(msgDelay),
      _idleCnt(idleCnt) {
    _consumer = new Consumer(_numMsg, _msgDelay);
    _consThread = new thread(&Consumer::mainLoop, _consumer);
}

Producer::~Producer() {
    _consThread->join();
    delete _consumer;
}

void Producer::mainLoop() {
    pthread_setname_np(pthread_self(), "Producer");

    for (unsigned int i = 0; i <= _numMsg; i++) {
        for (unsigned int j = 0; j < _idleCnt; j++) {
            unsigned long long sq = sqrt(j);
        }
        Msg *msg = new Msg(i,
                           static_cast<unsigned long long>(i) *
                               static_cast<unsigned long long>(i),
                           i);
        bool ret = _consumer->enqueue(msg);
        if (ret == false)
            cerr << "enqueue failed " << endl << flush;
    }
    _consumer->isRunning = 2;
    while (_consumer->isRunning == 2) {
    }
}

int main(int argc, char **argv) {
    unsigned int numMsg = 256U;
    useconds_t msgDelay = 0;
    unsigned int idleCnt = 256;
    if (argc > 1)
        numMsg = static_cast<unsigned int>(atoi(argv[1]));
    if (argc > 2)
        msgDelay = static_cast<useconds_t>(atoi(argv[2]));
    if (argc > 3)
        idleCnt = static_cast<unsigned int>(atoi(argv[3]));

    Producer *prod = new Producer(numMsg, msgDelay, idleCnt);
    thread *prodThread = new thread(&Producer::mainLoop, prod);
    prodThread->join();
    return 0;
}
