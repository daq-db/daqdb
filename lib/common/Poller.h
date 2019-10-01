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

#pragma once

#include <boost/circular_buffer.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>
#include <boost/thread/thread.hpp>
#include <boost/call_traits.hpp>
#include <boost/bind.hpp>
#include <boost/timer/timer.hpp>

#include "spdk/env.h"
#include "spdk/io_channel.h"
#include "spdk/queue.h"

#define DEQUEUE_RING_LIMIT 1024
#define DEQUEUE_BUFFER_LIMIT DEQUEUE_RING_LIMIT
#define DEQUEUE_BUFFER_QUANT 32

namespace DaqDB {

template <class T>
class BoundedBuffer {
public:
    typedef boost::circular_buffer<T> container_type;
    typedef typename container_type::size_type size_type;
    typedef typename container_type::value_type value_type;
    typedef typename boost::call_traits<value_type>::param_type param_type;

    explicit BoundedBuffer(size_type capacity) : m_unread(0), m_container(capacity) {}

    void pushFront(typename boost::call_traits<value_type>::param_type item) { // `param_type` represents the "best" way to pass a parameter of type `value_type` to a method.
        boost::mutex::scoped_lock lock(m_mutex);
        m_not_full.wait(lock, boost::bind(&BoundedBuffer<value_type>::isNotFull, this));
        m_container.push_front(item);
        ++m_unread;
        lock.unlock();
        m_not_empty.notify_one();
    }

    void popBack(value_type* pItem) {
        boost::mutex::scoped_lock lock(m_mutex);
        m_not_empty.wait(lock, boost::bind(&BoundedBuffer<value_type>::isNotEmpty, this));
        *pItem = m_container[--m_unread];
        lock.unlock();
        m_not_full.notify_one();
    }

private:
    BoundedBuffer(const BoundedBuffer&);              // Disabled copy constructor.
    BoundedBuffer& operator = (const BoundedBuffer&); // Disabled assign operator.

    bool isNotEmpty() const { return m_unread > 0; }
    bool isNotFull() const { return m_unread < m_container.capacity(); }

    size_type m_unread;
    container_type m_container;
    boost::mutex m_mutex;
    boost::condition m_not_empty;
    boost::condition m_not_full;
};


template <class T> class Poller {
  public:
    Poller() : requests(new T *[DEQUEUE_RING_LIMIT]) {
        //rqstRing = spdk_ring_create(SPDK_RING_TYPE_MP_SC, 4096 * 4, SPDK_ENV_SOCKET_ID_ANY);
        rqstBuffer = new BoundedBuffer<T *>(DEQUEUE_BUFFER_LIMIT);
    }
    virtual ~Poller() {
        //spdk_ring_free(rqstRing);
        delete[] requests;
        delete rqstBuffer;
    }

    virtual bool enqueue(T *rqst) {
        rqstBuffer->pushFront(rqst);
        //size_t count = spdk_ring_enqueue(rqstRing, (void **)&rqst, 1, 0);
        //return (count == 1);
        return true;
    }
    virtual void dequeue() {
        //requestCount = spdk_ring_dequeue(rqstRing, (void **)&requests[0], DEQUEUE_RING_LIMIT);
        unsigned short oneShot = DEQUEUE_BUFFER_QUANT;
        if ( requestCount > DEQUEUE_BUFFER_QUANT ) {
            oneShot = DEQUEUE_BUFFER_LIMIT;
        }
        unsigned short r_cnt = 0;
        for (  ; requestCount < oneShot ; requestCount++ ) {
            T *elem;
            rqstBuffer->popBack(&elem);
            assert(elem);
            requests[r_cnt++] = elem;
        }
        //assert(requestCount <= DEQUEUE_RING_LIMIT);
        assert(requestCount <= DEQUEUE_BUFFER_LIMIT);
    }

    virtual void process() = 0;

    //struct spdk_ring *rqstRing;
    BoundedBuffer<T *> *rqstBuffer;
    unsigned short requestCount = 0;
    T **requests;
};

} // namespace DaqDB
