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

#pragma once

#include <boost/circular_buffer.hpp>
#include <chrono>
#include <condition_variable>
#include <mutex>

namespace DaqDB {

const uint32_t dequeueBufferLimit = 1024;
const uint32_t dequeueBufferQuant = 1024;

template <class T> class BoundedBuffer {
  public:
    typedef boost::circular_buffer<T> ContainerType;

    explicit BoundedBuffer(size_t capacity, unsigned int timeout = 10)
        : _unread(0), _qContainer(capacity), _timeout(timeout) {}

    bool pushFront(T item) {
        std::unique_lock<std::mutex> lock(_cvMutex);
        const std::chrono::milliseconds timeout(_timeout);
        if (_cvNotFull.wait_for(lock, timeout, [this] {
                return BoundedBuffer<T>::isNotFull();
            }) == false) {
            lock.unlock();
            return false;
        }
        _qContainer.push_front(item);
        _unread++;
        lock.unlock();
        _cvNotEmpty.notify_all();
        return true;
    }

    bool popBack(T pItem) {
        std::unique_lock<std::mutex> lock(_cvMutex);
        const std::chrono::milliseconds timeout(_timeout);
        if (_cvNotEmpty.wait_for(lock, timeout, [this] {
                return BoundedBuffer<T>::isNotEmpty();
            }) == false) {
            lock.unlock();
            return false;
        }
        *pItem = _qContainer[--_unread];
        lock.unlock();
        _cvNotFull.notify_all();
        return true;
    }

    bool popBackVector(T pItem[], unsigned short *size) {
        std::unique_lock<std::mutex> lock(_cvMutex);
        const std::chrono::milliseconds timeout(_timeout);
        if (_cvNotEmpty.wait_for(lock, timeout, [this] {
                return BoundedBuffer<T>::isNotEmpty();
            }) == false) {
            lock.unlock();
            return false;
        }
        unsigned short i = 0;
        for (; _unread > 0 && i < dequeueBufferQuant;) {
            pItem[i++] = _qContainer[--_unread];
        }
        *size = i;
        lock.unlock();
        _cvNotFull.notify_all();
        return true;
    }

  private:
    BoundedBuffer(const BoundedBuffer &) = delete;
    BoundedBuffer &operator=(const BoundedBuffer &) = delete;

    bool isNotEmpty() const { return _unread > 0; }
    bool isNotFull() const { return _unread < _qContainer.capacity(); }

    size_t _unread;
    ContainerType _qContainer;
    std::mutex _cvMutex;
    std::condition_variable _cvNotEmpty;
    std::condition_variable _cvNotFull;
    unsigned int _timeout;
};

template <class T> class BlockingPoller {
  public:
    BlockingPoller(bool create_buf = true, unsigned int timeout = 10)
        : rqstBuffer(0), requests(new T *[dequeueBufferLimit]),
          createBuf(create_buf) {
        if (createBuf == true) {
            rqstBuffer = new BoundedBuffer<T *>(dequeueBufferLimit, timeout);
        }
    }
    virtual ~BlockingPoller() {
        delete rqstBuffer;
        delete[] requests;
    }

    bool init() {
        if (createBuf == false) {
            rqstBuffer = new BoundedBuffer<T *>(dequeueBufferLimit);
            return rqstBuffer ? true : false;
        }
        return true;
    }

    virtual bool enqueue(T *rqst) { return rqstBuffer->pushFront(rqst); }
    virtual bool dequeue() {
        bool ret = rqstBuffer->popBackVector(requests, &requestCount);
        assert(requestCount <= dequeueBufferLimit);
        return ret;
    }

    virtual void process() = 0;

    BoundedBuffer<T *> *rqstBuffer;
    unsigned short requestCount = 0;
    T **requests;
    bool createBuf;
};

} // namespace DaqDB
