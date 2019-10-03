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

//#define POLLER_USE_SPDK_RING

#ifndef POLLER_USE_SPDK_RING
#include <boost/circular_buffer.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>
#include <boost/thread/thread.hpp>
#include <boost/call_traits.hpp>
#include <boost/bind.hpp>
#include <boost/timer/timer.hpp>
#endif

#include "spdk/env.h"
#include "spdk/io_channel.h"
#include "spdk/queue.h"

#define DEQUEUE_RING_LIMIT 1024
#define DEQUEUE_BUFFER_LIMIT DEQUEUE_RING_LIMIT

namespace DaqDB {

#ifndef POLLER_USE_SPDK_RING

template <class T>
class BoundedBuffer {
public:
    typedef boost::circular_buffer<T> ContainerType;
    typedef typename ContainerType::size_type SizeType;
    typedef typename ContainerType::value_type ValueType;
    typedef typename boost::call_traits<ValueType>::param_type ParamType;

    explicit BoundedBuffer(SizeType capacity) : m_unread(0), m_container(capacity) {}

    void pushFront(typename boost::call_traits<ValueType>::param_type item) {
        boost::mutex::scoped_lock lock(m_mutex);
        m_not_full.wait(lock, boost::bind(&BoundedBuffer<ValueType>::isNotFull, this));
        m_container.push_front(item);
        ++m_unread;
        lock.unlock();
        m_not_empty.notify_one();
    }

    void popBack(ValueType* pItem) {
        boost::mutex::scoped_lock lock(m_mutex);
        m_not_empty.wait(lock, boost::bind(&BoundedBuffer<ValueType>::isNotEmpty, this));
        *pItem = m_container[--m_unread];
        lock.unlock();
        m_not_full.notify_one();
    }

    void popBackVector(ValueType pItem[], unsigned short *size) {
        boost::mutex::scoped_lock lock(m_mutex);
        m_not_empty.wait(lock, boost::bind(&BoundedBuffer<ValueType>::isNotEmpty, this));
        unsigned short i = 0;
        for ( ; m_unread > 0 ; ) {
            pItem[i++] = m_container[--m_unread];
        }
        *size = i;
        lock.unlock();
        m_not_full.notify_one();
    }

private:
    BoundedBuffer(const BoundedBuffer&) = delete;
    BoundedBuffer& operator = (const BoundedBuffer&) = delete;

    bool isNotEmpty() const { return m_unread > 0; }
    bool isNotFull() const { return m_unread < m_container.capacity(); }

    SizeType m_unread;
    ContainerType m_container;
    boost::mutex m_mutex;
    boost::condition m_not_empty;
    boost::condition m_not_full;
};
#endif

template <class T> class Poller {
  public:
    Poller(bool create_buf = true) :
#ifdef POLLER_USE_SPDK_RING
        rqstRing(0),
#else
        rqstBuffer(0),
#endif
        requests(new T *[DEQUEUE_RING_LIMIT]),
        createBuf(create_buf) {
        if ( createBuf == true ) {
#ifdef POLLER_USE_SPDK_RING
            rqstRing = spdk_ring_create(SPDK_RING_TYPE_MP_SC, 4096 * 4, SPDK_ENV_SOCKET_ID_ANY);
#else
            rqstBuffer = new BoundedBuffer<T *>(DEQUEUE_BUFFER_LIMIT);
#endif
        }
    }
    virtual ~Poller() {
#ifdef POLLER_USE_SPDK_RING
        spdk_ring_free(rqstRing);
#else
        delete rqstBuffer;
#endif
        delete[] requests;
    }

    bool init() {
        if ( createBuf == false ) {
#ifdef POLLER_USE_SPDK_RING
            rqstRing = spdk_ring_create(SPDK_RING_TYPE_MP_SC, 4096 * 4, SPDK_ENV_SOCKET_ID_ANY);
            return rqstRing ? true : false;
#else
            rqstBuffer = new BoundedBuffer<T *>(DEQUEUE_BUFFER_LIMIT);
            return rqstBuffer ? true : false;
#endif
        }
        return true;
    }

    virtual bool enqueue(T *rqst) {
#ifdef POLLER_USE_SPDK_RING
        size_t count = spdk_ring_enqueue(rqstRing, (void **)&rqst, 1, 0);
        return (count == 1);
#else
        rqstBuffer->pushFront(rqst);
        return true;
#endif
    }
    virtual void dequeue() {
#ifdef POLLER_USE_SPDK_RING
        requestCount = spdk_ring_dequeue(rqstRing, (void **)&requests[0], DEQUEUE_RING_LIMIT);
        assert(requestCount <= DEQUEUE_RING_LIMIT);
#else
        rqstBuffer->popBackVector(requests, &requestCount);
        assert(requestCount <= DEQUEUE_BUFFER_LIMIT);
#endif
    }

    virtual void process() = 0;

#ifdef POLLER_USE_SPDK_RING
    struct spdk_ring *rqstRing;
#else
    BoundedBuffer<T *> *rqstBuffer;
#endif
    unsigned short requestCount = 0;
    T **requests;
    bool createBuf;
};

} // namespace DaqDB
