#include <atomic>
#include <iostream>
#include <thread>

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
    Producer(unsigned int numMsg, useconds_t msgDelay);
    virtual ~Producer();

    void mainLoop();

  private:
    Consumer *_consumer;
    thread *_consThread;
    unsigned int _numMsg;
    useconds_t _msgDelay;
};

Producer::Producer(unsigned int numMsg, useconds_t msgDelay)
    : _consumer(0), _consThread(0), _numMsg(numMsg), _msgDelay(msgDelay) {
    _consumer = new Consumer(_numMsg, _msgDelay);
    _consThread = new thread(&Consumer::mainLoop, _consumer);
}

Producer::~Producer() {
    _consThread->join();
    delete _consumer;
}

void Producer::mainLoop() {
    for (unsigned int i = 0; i <= _numMsg; i++) {
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
    if (argc > 1)
        numMsg = static_cast<unsigned int>(atoi(argv[1]));
    if (argc > 2)
        msgDelay = static_cast<useconds_t>(atoi(argv[2]));
    Producer *prod = new Producer(numMsg, msgDelay);
    thread *prodThread = new thread(&Producer::mainLoop, prod);
    prodThread->join();
    return 0;
}
