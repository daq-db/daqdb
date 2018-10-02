/*
 * Copyright 2013-2020
 *      Advisor: Zhiling Lan(lan@iit.edu), Ioan Raicu(iraicu@cs.iit.edu)
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
 * This file is part of iit::cs550::finalproj(Diskon: Distributed tasK executiON
 *framework) Xiaobing Zhou(xzhou40@hawk.iit.edu) with nickname Xiaobingo, Hao
 *Chen(hchen71@hawk.iit.edu) with nickname Hchen, Iman
 *Sadooghi(isadoogh@iit.edu) with nickname Iman, Ke Wang(kwang22@hawk.iit.edu)
 *with nickname KWang.
 *
 * TSafeQueue.h
 *
 *  Created on: Nov 20, 2013
 *      Author: Xiaobingo, Hchen
 *      Contributor: Iman, KWang
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

#ifndef TSAFEQUEUE_H_
#define TSAFEQUEUE_H_

#include <pthread.h>
#include <queue>
//#include <mutex>
//#include <thread>
using namespace std;

#ifdef BQUEUE
#include <algorithm>
#include <boost/atomic.hpp>
#include <boost/lockfree/queue.hpp>
#include <boost/thread/thread.hpp>
#include <sys/types.h>
using namespace boost;
using namespace boost::lockfree;
using namespace std;
#endif

namespace iit {
namespace cs550 {
namespace finalproj {

/*
 *
 */
template <typename T> class TSafeQueue {
  public:
    TSafeQueue();
    virtual ~TSafeQueue();

  public:
    size_t size();

    bool push(const T &element);

    bool pop(T &element);

    void pop_n(int n);

    void pop_all();

  private:
    void init();

  private:
    std::queue<T> _queue;
    // std::mutex _mutex;
    pthread_mutex_t _mutex;
};

#ifdef BQUEUE
template <typename T> class TSafeQueue {
  public:
    TSafeQueue();
    TSafeQueue(int reservation);
    virtual ~TSafeQueue();

  public:
    bool empty();

    size_t size();

    bool push(const T &element);

    bool pop(T &element);

    bool pop_n(uint n);

    bool pop_all();

  private:
    boost::atomic_int _count;
    boost::lockfree::queue<T> _queue;
};
#endif

} // namespace finalproj
/* namespace finalproj */
} /* namespace cs550 */
} /* namespace iit */
#endif /* TSAFEQUEUE_H_ */
