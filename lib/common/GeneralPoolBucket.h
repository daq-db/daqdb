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

#include "AllocStrategy.h"
#include "GeneralPoolBuffer.h"

#include <mutex>
#include <shared_mutex>

typedef std::mutex Lock;
typedef std::unique_lock<Lock> WriteLock;
typedef std::unique_lock<Lock> ReadLock;

#include <iostream>
using namespace std;

#ifdef _MEM_STATS_
#include <stdio.h>
#include <string.h>
#include <unistd.h>

// extern "C" void U_STACK_TRACE(void);
#endif

namespace DaqDB {
#ifdef _MEM_STATS_
extern Lock gl_mem_mutex;
#endif
const unsigned int MAX_BUCKET_BUFFERS = 256;

template <class T, class Alloc> class GeneralPoolBucket {
  public:
    GeneralPoolBucket();
    virtual ~GeneralPoolBucket();
    T *get();
    T *getNoLock();
    void put(T *obj_);
    void putNoLock(T *obj_);
    void make(unsigned int count_);
    unsigned int size();
    bool isFull();
    bool isEmpty();
    void dump();
    void dumpNoLock();
    void makeStamp();
    void allocate(unsigned int howMany_);
    void setStrategy(const AllocStrategy &strat_);
    void makeBuffer();
    void stats(FILE &file_);
    void traceStack();
    void setName(const char *_name);
    void setRttiName(const char *_rttiName);

    const unsigned int getQuantInc() const;
    void setQuantInc(unsigned int value);

    const unsigned int getQuantDec() const;
    void setQuantDec(unsigned int value);

    const unsigned int getNumBuffers() const;
    void setNumBuffers(unsigned int value);

    const unsigned int getBucketNum() const;
    void setBucketNum(unsigned int value);

    const unsigned int getTotalMax() const;
    void setTotalMax(unsigned int value);

    const unsigned int getCurrentMax() const;
    void setCurrentMax(unsigned int value);

    void setStackTraceFile(FILE *value);

  private:
    GeneralPoolBucket(const GeneralPoolBucket<T, Alloc> &right);
    GeneralPoolBucket<T, Alloc> &
    operator=(const GeneralPoolBucket<T, Alloc> &right);

    unsigned int head;
    unsigned int tail;
    bool full;
    bool empty;
    unsigned int quantInc;
    unsigned int quantDec;
    unsigned int numBuffers;
    unsigned int bucketNum;
    unsigned int stamp;
    unsigned int totalMax;
    unsigned int currentMax;
    Lock mutex;
    unsigned int objSize;
    unsigned int padding;
    FILE *stackTraceFile;
    char name[32];
    char rttiName[132];

    GeneralPoolBuffer<T, Alloc> *buffers[MAX_BUCKET_BUFFERS];

#ifdef _MEM_STATS_
    unsigned long getRequests;
    unsigned long putRequests;
    unsigned long memAllocs;
    unsigned long memDeallocs;
    unsigned long stackTraceThreshold;
    bool stackTrace;
    char tracePoolName[64];
    bool tracePoolNameFlag;

  public:
    void setStackTraceThreshold(unsigned long _stackTraceThreshold) {
        stackTraceThreshold = _stackTraceThreshold;
    }
#endif

  private:
};

template <class T, class Alloc>
inline GeneralPoolBucket<T, Alloc>::GeneralPoolBucket()
    : head(0), tail(0), full(false), empty(true), quantInc(0), quantDec(0),
      numBuffers(0), stamp(0), totalMax(MAX_BUCKET_BUFFERS * MAX_BUFFER_SLOTS),
      currentMax(0), objSize(Alloc::objectSize()), padding(2 * OBJ_PADDING),
      stackTraceFile(0)
#ifdef _MEM_STATS_
      ,
      getRequests(0UL), putRequests(0UL), memAllocs(0UL), memDeallocs(0UL),
      stackTraceThreshold(1000UL), stackTrace(false), tracePoolNameFlag(false)
#endif
{
#ifdef _MM_DEBUG_
#ifndef _MM_GMP_ON_
    cout << "GeneralPoolBucket<T,Alloc>::GeneralPoolBucket() " << endl << flush;
#else
    fprintf(stderr, "GeneralPoolBucket<T,Alloc>::GeneralPoolBucket()\n");
    fflush(stderr);
#endif
#endif

    // NULL out name and rttiName
    name[0] = '\0';
    rttiName[0] = '\0';

    // check if stack trace is true
#ifdef _MEM_STATS_
    char *sf = getenv("MEM_STACK_TRACE");
    if (sf) {
        if (atoi(sf) == 1)
            stackTrace = true;
    }

    // set the threshold
    // get the threshold for stack trace to kick in
    char *st = getenv("MEM_LEAK_THRESHOLD");
    // unsigned long stack_th = 0;
    if (st) {
        stackTraceThreshold = (unsigned long)atol(st);
    }

    // check if the pool name for stack tracing is set
    char *tpn = getenv("MEM_TRACE_POOL_NAME");
    if (tpn) {
        strncpy(tracePoolName, tpn, sizeof(tracePoolName) - 1);
        tracePoolName[sizeof(tracePoolName) - 1] = '\0';
        tracePoolNameFlag = true;
    }
#endif
}

template <class T, class Alloc>
inline GeneralPoolBucket<T, Alloc>::~GeneralPoolBucket() {}

template <class T, class Alloc> inline T *GeneralPoolBucket<T, Alloc>::get() {
    // set the lock on
    WriteLock r_lock(mutex);
    return getNoLock();
}

template <class T, class Alloc>
inline T *GeneralPoolBucket<T, Alloc>::getNoLock() {
#ifdef _MEM_STATS_
    // check if the stack trace file is set
    if (stackTrace == true) {
        if (memAllocs > stackTraceThreshold) {
            if (tracePoolNameFlag == false ||
                !strncmp(rttiName, tracePoolName, sizeof(tracePoolName)))
                traceStack();
        }
    }
#endif

    // make more object if size drops below 10
    if (empty == true) {
        make(quantInc);
    }

#ifdef _MEM_STATS_
    // increment get counter
    getRequests++;
#endif

    // compute buffer index and offset
    unsigned int buf_idx = tail / MAX_BUFFER_SLOTS;
    unsigned int slot_idx = tail % MAX_BUFFER_SLOTS;

    // get an object
    T *tmp_obj = buffers[buf_idx]->slots[slot_idx];

    // sanity check
    if (!tmp_obj) {
        // error - abort
#ifndef _MM_GMP_ON_
        cerr << "Null returned from slot, b_idx: " << buf_idx
             << " s_idx: " << slot_idx << endl
             << flush;
        cerr << "-> Bucket ";
#else
        fprintf(stderr, "Null returned from slot, b_idx: %d s_idx: %d\n",
                buf_idx, slot_idx);
        fflush(stderr);
        fprintf(stderr, "->Bucket ");
#endif
        dumpNoLock();
        abort();
    }

    // NULL out the slot
    buffers[buf_idx]->slots[slot_idx] = 0;

    // advance the tail, there are objects there because of they never drop
    // under 10
    tail++;
    tail %= currentMax;

    // if tail cought up with the head the bucket is empty
    // it's however impossible since the capacity never drops under 10
    if (tail == head) {
        empty = true;
    }

    // it's not full any mor esince we've taken one out
    full = false;

#ifdef _MM_DEBUG_
#ifndef _MM_GMP_ON_
    unsigned int o_stamp = *(unsigned int *)((char *)tmp_obj + objSize);
#else
    unsigned int o_stamp = *(unsigned int *)((char *)tmp_obj - OBJ_PADDING);
#endif

    unsigned int o_bucket_size = o_stamp & BUCKET_SIZE_MASK;
    unsigned int o_bucket_number =
        (o_stamp >> BUCKET_NUMBER_SHIFT) & BUCKET_NUMBER_MASK;

#ifndef _MM_GMP_ON_
    cout << "GeneralPoolBucket<T,Alloc>::get() "
         << " tail: " << tail << " head: " << head
         << " currentMax: " << currentMax << " numBuffers: " << numBuffers
         << " stamp: " << o_stamp << " full: " << full << " empty: " << empty
         << " bs: " << o_bucket_size << " bn: " << o_bucket_number
         << " buf_idx: " << buf_idx << " slot_idx: " << slot_idx
         << " tmp_obj: " << tmp_obj << endl
         << flush;
#endif
#endif
    // return object
    return tmp_obj;
}

template <class T, class Alloc>
inline void GeneralPoolBucket<T, Alloc>::put(T *obj_) {
    // set the lock on
    WriteLock r_lock(mutex);
    putNoLock(obj_);
}

template <class T, class Alloc>
inline void GeneralPoolBucket<T, Alloc>::putNoLock(T *obj_) {
#if _MM_DOUBLE_DEL_CHECK_
    // check if the pointer is already in the pool
    if (empty != true) {
        // compute buffer index and offset
        unsigned int o_buf_idx = cnt / MAX_BUFFER_SLOTS;
        unsigned int o_slot_idx = cnt % MAX_BUFFER_SLOTS;

        // get an object
        T *tmp_obj = buffers[o_buf_idx]->slots[o_slot_idx];

        // advance the tail, there are objects there because of they never drop
        // under 10
        cnt++;
        cnt %= currentMax;
        if (cnt == head)
            break;

        // check if the pointer is not NULL
        if (tmp_obj == obj_) {
#ifndef _MM_GMP_ON_
            cerr << "Object already returned, o_b_idx: " << o_buf_idx
                 << " o_s_idx: " << o_slot_idx << endl
                 << flush;
            cerr << "-> Bucket ";
#else
            fprintf(stderr,
                    "Object already returned, o_b_idx: %d o_s_idx: %d\n",
                    o_buf_idx, o_slot_idx);
            fflush(stderr);
            fprintf(stderr, "-> Bucket");
#endif
            dumpNoLock();
            abort();
        }
    }
#endif

    // make buffers if necessary
    if (full == true || !numBuffers) {
        makeBuffer();
        if (full == true) {
            return;
        }
    }

#ifdef _MEM_STATS_
    // increment put counter
    putRequests++;
#endif

    // compute buffer index and offset
    unsigned int buf_idx = head / MAX_BUFFER_SLOTS;
    unsigned int slot_idx = head % MAX_BUFFER_SLOTS;

    // if the slot is not NULL - error
    if (buffers[buf_idx]->slots[slot_idx]) {
#ifndef _MM_GMP_ON_
        cerr << "Slot should be NULL, b_idx: " << buf_idx
             << " s_idx: " << slot_idx << endl
             << flush;
        cerr << "-> Bucket ";
#else
        fprintf(stderr, "Slot should be NULL, b_idx: %d s_idx: %d\n", buf_idx,
                slot_idx);
        fflush(stderr);
        fprintf(stderr, "-> Bucket");
#endif
        dumpNoLock();
        abort();
    }

    // return it
    buffers[buf_idx]->slots[slot_idx] = obj_;

    // advance the head
    head++;
    head %= currentMax;

    // check if it's full
    if (head == tail) {
        full = true;
    }

    // it's not empty since we've just put one in
    empty = false;

#ifdef _MM_DEBUG_
#ifndef _MM_GMP_ON_
    unsigned int o_stamp = *(unsigned int *)((char *)obj_ + objSize);
#else
    unsigned int o_stamp = *(unsigned int *)((char *)obj_ - OBJ_PADDING);
#endif

    unsigned int o_bucket_size = o_stamp & BUCKET_SIZE_MASK;
    unsigned int o_bucket_number =
        (o_stamp >> BUCKET_NUMBER_SHIFT) & BUCKET_NUMBER_MASK;

#ifndef _MM_GMP_ON_
    cout << "GeneralPoolBucket<T,Alloc>::put() "
         << " obj_: " << obj_ << " tail: " << tail << " head: " << head
         << " currentMax: " << currentMax << " numBuffers: " << numBuffers
         << " stamp: " << o_stamp << " full: " << full << " empty: " << empty
         << " bs: " << o_bucket_size << " bn: " << o_bucket_number
         << " buf_idx: " << buf_idx << " slot_idx: " << slot_idx << endl
         << flush;
#endif
#endif

    return;
}

#if 0
template<class T, class Alloc>
inline void GeneralPoolBucket<T, Alloc>::putNoLock(T *obj_)
{
    // make buffers if necessary
    if (full == true || !numBuffers) {
        makeBuffer();
        if (full == true) {
            return;
        }
    }

    // get a snapshot of this bucket
    unsigned int buf_idx = head / MAX_BUFFER_SLOTS;
    unsigned int slot_idx = head % MAX_BUFFER_SLOTS;

    // if the slot is not NULL - error
    if (buffers[buf_idx]->slots[slot_idx]) {
#ifndef _MM_GMP_ON_
        cerr << "Slot (no lock) should be NULL, b_idx: " << buf_idx
                << " s_idx: " << slot_idx << endl << flush;
        cerr << "-> Bucket ";
#else
        fprintf(stderr, "Slot (no lock) should be NULL, b_idx: %d s_idx: %d\n", buf_idx, slot_idx);
        fflush(stderr);
        fprintf(stderr, "-> Bucket");
#endif
        dumpNoLock();
        abort();
    }

    // return it
    buffers[buf_idx]->slots[slot_idx] = obj_;

    // advance the head
    head++;
    head %= currentMax;

    // check if it's full
    if (head == tail) {
        full = true;
    }

    // it's not empty since we've just put one in
    empty = false;

#ifdef _MM_DEBUG_

#ifndef _MM_GMP_ON_
    unsigned int o_stamp = *(unsigned int *)((char *)obj_ + objSize);
#else
    unsigned int o_stamp = *(unsigned int *)((char *)obj_ - OBJ_PADDING);
#endif

    unsigned int o_bucket_size = o_stamp & BUCKET_SIZE_MASK;
    unsigned int o_bucket_number = (o_stamp >> BUCKET_NUMBER_SHIFT) & BUCKET_NUMBER_MASK;

#ifndef _MM_GMP_ON_
    cout << "GeneralPoolBucket<T,Alloc>::putNoLock() "
    << " obj_: " << obj_
    << " tail: " << tail
    << " head: " << head
    << " currentMax: " << currentMax
    << " numBuffers: " << numBuffers
    << " stamp: " << stamp
    << " full: " << full
    << " empty: " << empty
    << " bs: " << o_bucket_size
    << " bn: " << o_bucket_number
    << " buf_idx: " << buf_idx
    << " slot_idx: " << slot_idx
    << endl << flush;
#endif
#endif

    return;
}
#endif

template <class T, class Alloc>
inline void GeneralPoolBucket<T, Alloc>::make(unsigned int count_) {
    for (unsigned int cnt = 0; cnt < count_; cnt++) {
        // make new one
        T *tmp_obj = Alloc::New(padding);

#ifdef _MEM_STATS_
        // increment alloc/new counter
        memAllocs++;
#endif

#ifndef _MM_GMP_ON_
        // back stamp it
        *((unsigned int *)((char *)tmp_obj + objSize)) = stamp;
#else
        // front stamp it
        *((unsigned int *)((char *)tmp_obj - OBJ_PADDING)) = stamp;
#endif

#ifdef _MM_DEBUG_
#ifndef _MM_GMP_ON_
        unsigned int o_stamp = *(unsigned int *)((char *)tmp_obj + objSize);
#else
        unsigned int o_stamp = *(unsigned int *)((char *)tmp_obj - OBJ_PADDING);
#endif

#ifndef _MM_GMP_ON_
        cout << "GeneralPoolBucket<T,Alloc>::make() "
             << " tmp_obj: " << tmp_obj << " tail: " << tail
             << " head: " << head << " currentMax: " << currentMax
             << " numBuffers: " << numBuffers << " stamp: " << stamp
             << " os: " << o_stamp << " full: " << full << " empty: " << empty
             << endl
             << flush;
#else
        fprintf(stderr,
                "GeneralPoolBucket<T,Alloc>::make() tmp_obj: %x tail: %d head: "
                "%d currentMax: %d numBuffers: %d stamp: %d os: %d full: %d "
                "empty: %d\n",
                tmp_obj, tail, head, currentMax, numBuffers, stamp, o_stamp,
                full, empty);
        fflush(stderr);
#endif
#endif

        // put it in
        putNoLock(tmp_obj);
    }
}

template <class T, class Alloc>
inline unsigned int GeneralPoolBucket<T, Alloc>::size() {
#ifdef _MM_DEBUG_
#ifndef _MM_GMP_ON_
    cout << "GeneralPoolBucket<T,Alloc>::size() "
         << " full: " << full << " tail: " << tail << " head: " << head
         << " currentMax: " << currentMax << endl
         << flush;
#endif
#endif

    if (full == true) {
        return currentMax;
    }

    if (head >= tail) {
        return head - tail;
    }

    return currentMax - (tail - head);
}

template <class T, class Alloc>
inline bool GeneralPoolBucket<T, Alloc>::isFull() {
    return full;
}

template <class T, class Alloc>
inline bool GeneralPoolBucket<T, Alloc>::isEmpty() {
    return empty;
}

template <class T, class Alloc>
inline void GeneralPoolBucket<T, Alloc>::dump() {
    // set the lock on
    ReadLock mutex;

    // call no lock version
    dumpNoLock();
}

template <class T, class Alloc>
inline void GeneralPoolBucket<T, Alloc>::dumpNoLock() {
#ifndef _MM_GMP_ON_
    cerr << " , obj size: " << objSize << " num of buffers: " << numBuffers
         << " objs in: " << size() << " tail: " << tail << " head: " << head
         << endl
         << flush;
#else
    fprintf(
        stderr,
        " , obj size: %d num of buffers: %d objs in: %d tail: %d head: %d\n",
        objSize, numBuffers, size(), tail, head);
    fflush(stderr);
#endif

    for (unsigned int cnt = tail; cnt != head;) {
        // compute buffer index and offset
        unsigned int buf_idx = cnt / MAX_BUFFER_SLOTS;
        unsigned int slot_idx = cnt % MAX_BUFFER_SLOTS;

        // get an object
        T *tmp_obj = buffers[buf_idx]->slots[slot_idx];

        // advance the tail, there are objects there because of they never drop
        // under 10
        cnt++;
        cnt %= currentMax;

        // check if the pointer is not NULL
        if (tmp_obj) {
#ifndef _MM_GMP_ON_
            unsigned int o_stamp =
                *((unsigned int *)((char *)tmp_obj + objSize));
#else
            unsigned int o_stamp =
                *((unsigned int *)((char *)tmp_obj - OBJ_PADDING));
#endif

            unsigned int o_bucket_size = o_stamp & BUCKET_SIZE_MASK;
            unsigned int o_bucket_number =
                (o_stamp >> BUCKET_NUMBER_SHIFT) & BUCKET_NUMBER_MASK;

#ifndef _MM_GMP_ON_
            cout << " -> Obj, cnt: " << cnt << " o_stamp: " << o_stamp
                 << " o_bs: " << o_bucket_size << " o_bn: " << o_bucket_number
                 << endl
                 << flush;
#else
            fprintf(stderr, " -> Obj, cnt: %d o_stamp: %d o_bs: %d o_bn: %d\n",
                    cnt, o_stamp, o_bucket_size, o_bucket_number);
            fflush(stderr);
#endif
        } else
#ifndef _MM_GMP_ON_
            cout << " -> Obj NULL entry: " << cnt << endl << flush;
#else
            fprintf(stderr, " -> Obj NULL entry: %d\n", cnt);
        fflush(stderr);
#endif
    }
}

template <class T, class Alloc>
inline void GeneralPoolBucket<T, Alloc>::makeStamp() {
    stamp = (bucketNum << BUCKET_NUMBER_SHIFT) + objSize;

#ifdef _MM_DEBUG_
#ifndef _MM_GMP_ON_
    cout << "GeneralPoolBucket<T,Alloc>::makeStamp() "
         << " stamp: " << stamp << endl
         << flush;
#endif
#endif
}

template <class T, class Alloc>
inline void GeneralPoolBucket<T, Alloc>::allocate(unsigned int howMany_) {
    // set the lock on
    ReadLock mutex;

    // make objects
    make(howMany_);

#ifdef _MM_DEBUG_
#ifndef _MM_GMP_ON_
    cout << "GeneralPoolBucket<T,Alloc>::allocate() "
         << " howMany_: " << howMany_ << endl
         << flush;
#endif
#endif
}

template <class T, class Alloc>
inline void
GeneralPoolBucket<T, Alloc>::setStrategy(const AllocStrategy &strat_) {
    // determine number of objs to preallocate
    unsigned int preall = strat_.preallocateCount() / Alloc::objectSize();

    // set the inc quant
    quantInc = strat_.incrementQuant() / Alloc::objectSize();

    if (quantInc <= strat_.minIncQuant()) {
        quantInc = strat_.minIncQuant();
    }

    if (quantInc >= strat_.maxIncQuant()) {
        quantInc = strat_.maxIncQuant();
    }

    // set the dec quant
    quantDec = strat_.decrementQuant() / Alloc::objectSize();

    if (quantDec <= strat_.minDecQuant()) {
        quantDec = strat_.minDecQuant();
    }

    if (quantDec >= strat_.maxDecQuant()) {
        quantDec = strat_.maxDecQuant();
    }

    // now preallocate
    allocate(preall);

    return;
}

template <class T, class Alloc>
inline void GeneralPoolBucket<T, Alloc>::makeBuffer() {
    // out of space - abort
    if (numBuffers >= MAX_BUCKET_BUFFERS) {
#ifndef _MM_GMP_ON_
        cerr << "No more buffers can be allocated "
             << " See Jan Lisowiec" << endl
             << flush;
#else
        fprintf(stderr, "No more buffers can be allocated See Jan Lisowiec\n");
        fflush(stderr);
#endif
        dump();
        abort();
    }

#ifdef _MM_DEBUG_
    // check all the pointers
    for (unsigned int cnt = tail; full == true;) {
        // compute buffer index and offset
        unsigned int buf_idx = cnt / MAX_BUFFER_SLOTS;
        unsigned int slot_idx = cnt % MAX_BUFFER_SLOTS;

        // get an object
        T *tmp_obj = buffers[buf_idx]->slots[slot_idx];

        // advance the tail, there are objects there because of they never drop
        // under 10
        cnt++;
        cnt %= currentMax;

        if (cnt == tail) {
            break;
        }

        // check if the pointer is not NULL
        if (tmp_obj) {
#ifndef _MM_GMP_ON_
            unsigned int o_stamp =
                *((unsigned int *)((char *)tmp_obj + objSize));
#else
            unsigned int o_stamp =
                *(unsigned int *)(((char *)tmp_obj - OBJ_PADDING));
#endif

            unsigned int o_bucket_size = o_stamp & BUCKET_SIZE_MASK;
            unsigned int o_bucket_number =
                (o_stamp >> BUCKET_NUMBER_SHIFT) & BUCKET_NUMBER_MASK;

            if (o_bucket_number != bucketNum || o_bucket_size != objSize)
#ifndef _MM_GMP_ON_
                cerr << " ===> Wrong entry in bucket: " << bucketNum
                     << " o_bs: " << o_bucket_size
                     << " o_bn: " << o_bucket_number << " entry: " << cnt
                     << endl
                     << flush;
#else
                fprintf(stderr, " ===> Wrong entry in bucket: %d o_bs: %d\n",
                        bucketNum, o_bucket_size);
            fflush(stderr);
#endif
        } else {
#ifndef _MM_GMP_ON_
            cout << " ===> Obj NULL entry: " << cnt << " tail: " << tail
                 << " head: " << head << endl
                 << flush;
#else
            fprintf(stderr, " ===> Obj NULL entry: %d tail: %d head: %d\n", cnt,
                    tail, head);
            fflush(stderr);
#endif
        }
    }
#endif

    // we have to insert the buffer at the head/MAX_BUFFER_SLOTS position
    unsigned int insert_pos = head / MAX_BUFFER_SLOTS;

    // first shift the whole thing by numBuffers - insert_pos
    memmove((void *)&buffers[insert_pos + 1], (void *)&buffers[insert_pos],
            (size_t)((numBuffers - insert_pos) *
                     sizeof(GeneralPoolBuffer<T, Alloc> *)));

    // create new buffer
    buffers[insert_pos] = new GeneralPoolBuffer<T, Alloc>(bucketNum);

    // fix the new buffer by rellocating slots from moved buffer
    buffers[insert_pos]->rellocate(buffers[insert_pos + 1],
                                   head % MAX_BUFFER_SLOTS);

    // zero out rellocated slots in moved buffer
    buffers[insert_pos + 1]->zeroOut(head % MAX_BUFFER_SLOTS);

#ifdef _MM_DEBUG_
#ifndef _MM_GMP_ON_
    cout << "makeBuffer insert_pos: " << insert_pos << " tail: " << tail
         << " head: " << head << " numBuffers: " << numBuffers
         << " bucketNum: " << bucketNum << " full: " << full
         << " empty: " << empty << endl
         << flush;
#endif
#endif

    // increamement numBuffers
    numBuffers++;

    // recalculate currentMax
    currentMax += MAX_BUFFER_SLOTS;

    // adjust tail
    tail += MAX_BUFFER_SLOTS;
    tail %= currentMax;

    // most definitely it's not full anymore
    full = false;

#ifdef _MM_DEBUG_
#ifndef _MM_GMP_ON_
    cout << "GeneralPoolBucket<T,Alloc>::makeBuffer() "
         << " bucketNum: " << bucketNum << " numBuffers: " << numBuffers
         << " insert_pos: " << insert_pos << " currentMax: " << currentMax
         << endl
         << flush;
#endif
#endif
}

template <class T, class Alloc>
inline void GeneralPoolBucket<T, Alloc>::stats(FILE &file_) {
#ifdef _MEM_STATS_
    // put the lock on
    ReadLock mutex;

    // save the values in local vars
    unsigned int bucket_num = bucketNum;
    unsigned long get_cnt = getRequests;
    unsigned long put_cnt = putRequests;
    unsigned long mem_a_cnt = memAllocs;
    unsigned long mem_d_cnt = memDeallocs;
    unsigned long curr_size = size();

    fprintf(&file_,
            " -> Bucket num: %d getCnt: %ld putCnt: %ld memAllocs: %ld "
            "memDeallocs: %ld size: %ld",
            bucket_num, get_cnt, put_cnt, mem_a_cnt, mem_d_cnt, curr_size);
#endif
}

template <class T, class Alloc>
inline void GeneralPoolBucket<T, Alloc>::traceStack() {
#ifdef _MEM_STATS_

    // lock the global mutex
    ReadLock r_lock(gl_mem_mutex);

#if 0

    // check if the file is opened
    if ( !stackTraceFile ) {
        // open stackTraceFile
        char name_buf[64];
        sprintf(name_buf, "mem_mgmt_stack_trace_%s_%d_%d", rttiName, bucketNum, getpid());
        stackTraceFile = fopen(name_buf, "w");

        // check if OK
        if ( !stackTraceFile ) {
            fprintf(stderr, "%s\n", strerror(errno));
            return;
        }
    }

    // get the file descriptor for the trace file
    int fd = fileno(stackTraceFile);

    // flush it out
    fflush(stackTraceFile);

    // switch files
    int fd2 = dup(STDERR_FILENO);
    if ( fd2 == -1 && errno == EMFILE ) {
        fprintf(stderr, "Error dup(), errno %d\n", errno);
        fflush(stderr);
        return;
    }

    int err = -1;
    for (;; ) {
        err = dup2(fd, STDERR_FILENO);
        if ( err >= 0 ) {
            break;
        }

        // check for EINTR
        if ( err == -1 && errno != EINTR ) {
            fprintf(stderr, "(%d) Error dup2(), errno %d\n", __LINE__, errno);
            fflush(stderr);
            close(fd2);
            return;
        }
    }

#endif

    // print a header
    // fprintf(stderr, "(*) **** %d traceStack() for %s shows ****\n",
    // memAllocs, rttiName);

    // trace the stack
    // U_STACK_TRACE();

    // flush stderr
    // fflush(stderr);

#if 0
    // flush stderr
    fflush(stderr);

    // restore files
    for (;; ) {
        if ( (fd = dup2(fd2, STDERR_FILENO)) >= 0 )
        break;

        // check for EINTR
        if ( fd == -1 && errno == EINTR ) {
            continue;
        }

        if ( fd == -1 && errno != EINTR ) {
            fprintf(stderr, "(%d) Error dup2(), errno %d\n", __LINE__, errno);
            fflush(stderr);
            break;
        }
    }

    close(fd2);

#endif
#endif

    return;
}

template <class T, class Alloc>
inline void GeneralPoolBucket<T, Alloc>::setName(const char *_name) {
    strncpy(name, _name, sizeof(name) - 1);
    name[sizeof(name) - 1] = '\0';
}

template <class T, class Alloc>
inline void GeneralPoolBucket<T, Alloc>::setRttiName(const char *_rttiName) {
    strncpy(rttiName, _rttiName, sizeof(rttiName) - 1);
    rttiName[sizeof(rttiName) - 1] = '\0';
}

template <class T, class Alloc>
inline const unsigned int GeneralPoolBucket<T, Alloc>::getQuantInc() const {
    return quantInc;
}

template <class T, class Alloc>
inline void GeneralPoolBucket<T, Alloc>::setQuantInc(unsigned int value) {
    quantInc = value;
}

template <class T, class Alloc>
inline const unsigned int GeneralPoolBucket<T, Alloc>::getQuantDec() const {
    return quantDec;
}

template <class T, class Alloc>
inline void GeneralPoolBucket<T, Alloc>::setQuantDec(unsigned int value) {
    quantDec = value;
}

template <class T, class Alloc>
inline const unsigned int GeneralPoolBucket<T, Alloc>::getNumBuffers() const {
    return numBuffers;
}

template <class T, class Alloc>
inline void GeneralPoolBucket<T, Alloc>::setNumBuffers(unsigned int value) {
    numBuffers = value;
}

template <class T, class Alloc>
inline const unsigned int GeneralPoolBucket<T, Alloc>::getBucketNum() const {
    return bucketNum;
}

template <class T, class Alloc>
inline void GeneralPoolBucket<T, Alloc>::setBucketNum(unsigned int value) {
    bucketNum = value;
}

template <class T, class Alloc>
inline const unsigned int GeneralPoolBucket<T, Alloc>::getTotalMax() const {
    return totalMax;
}

template <class T, class Alloc>
inline void GeneralPoolBucket<T, Alloc>::setTotalMax(unsigned int value) {
    totalMax = value;
}

template <class T, class Alloc>
inline const unsigned int GeneralPoolBucket<T, Alloc>::getCurrentMax() const {
    return currentMax;
}

template <class T, class Alloc>
inline void GeneralPoolBucket<T, Alloc>::setCurrentMax(unsigned int value) {
    currentMax = value;
}

template <class T, class Alloc>
inline void GeneralPoolBucket<T, Alloc>::setStackTraceFile(FILE *value) {
    stackTraceFile = value;
}
} // namespace DaqDB
