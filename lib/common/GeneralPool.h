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

#include <assert.h>
#include <mutex>

#include "ClassAlloc.h"
#include "ClassMemoryAlloc.h"
#include "DefaultAllocStrategy.h"
#include "GeneralPoolBase.h"
#include "GeneralPoolBucket.h"
#include "GlobalMemoryAlloc.h"

#include "PoolManager.h"

#include <typeinfo>

namespace MemMgmt {
const unsigned int MAX_POOL_BUCKETS = 64;

template <class T, class Alloc> class GeneralPool : public GeneralPoolBase {
  public:
    friend class MemMgr;

  public:
    GeneralPool(unsigned short id_, const char *name_ = 0,
                const AllocStrategy &strategy_ = DefaultAllocStrategy());
    GeneralPool(const char *name_ = 0,
                const AllocStrategy &strategy_ = DefaultAllocStrategy());
    virtual ~GeneralPool();
    virtual void manage();
    T *get();
    T *get(unsigned int pool_);
    void put(T *obj_);
    void put(T *obj_, unsigned int pool_);
    virtual void dump();
    virtual void stats(FILE &file_);
    const AllocStrategy *getStrategy() const;
    void setStrategy(AllocStrategy *value);
    unsigned int getPoolSize();

  protected:
    GeneralPool(const GeneralPool<T, Alloc> &right);
    GeneralPool<T, Alloc> &operator=(const GeneralPool<T, Alloc> &right);

  private:
    Lock mutex;
    AllocStrategy *strategy;
    unsigned int counter;
    unsigned int objSize;
    // gint                        bucket_locks[MAX_POOL_BUCKETS];
    GeneralPoolBucket<T, Alloc> buckets[MAX_POOL_BUCKETS];
};

template <class T, class Alloc>
unsigned int GeneralPool<T, Alloc>::getPoolSize() {
    return MAX_POOL_BUCKETS;
}

template <class T, class Alloc>
inline GeneralPool<T, Alloc>::GeneralPool(unsigned short id_, const char *name_,
                                          const AllocStrategy &strategy_)
    : GeneralPoolBase(id_, name_), strategy(strategy_.copy()), counter(0),
      objSize(Alloc::objectSize()) {
    // set the rtti name
    const char *alloc_name = Alloc::getName();
    char comb_name[128];
    const type_info &ti = typeid(T);
    sprintf(comb_name, "%s::%s", alloc_name, ti.name());

#ifdef _MM_DEBUG_
#ifndef _MM_GMP_ON_
    cout < "NAME: " << ti.name() << " comb name: " << comb_name << endl
                    << flush;
#else
    printf(stderr, "NAME: %s comb_name: %s\n", ti.name(), comb_name);
#endif
#endif

    // set the rttiName in the base class
    setRttiName(comb_name);

#ifdef _MEM_STATS_
    // get the threshold for stack trace to kick in
    char *st = getenv("MEM_LEAK_THRESHOLD");
    unsigned long stack_th = 0;
    if (st) {
        stack_th = (unsigned long)atol(st);
    }
#endif

    for (unsigned int cnt = 0; cnt < MAX_POOL_BUCKETS; cnt++) {
        buckets[cnt].setBucketNum(cnt);
        buckets[cnt].makeStamp();
        buckets[cnt].setName(name);
        buckets[cnt].setRttiName(rttiName);

#ifdef _MEM_STATS_
        // buckets[cnt].setStackTraceFile(stackTraceFile);
        buckets[cnt].setStackTraceThreshold(stack_th);
#endif

        // execute the strategy
        buckets[cnt].setStrategy(*strategy);
    }

    // register w/PoolManager
    PoolManager &pm = PoolManager::getInstance();
    pm.registerPool(this);

#ifdef _MM_DEBUG_
#ifndef _MM_GMP_ON_
    cout << "GeneralPool<T,Alloc>::GeneralPool "
         << " name: " << getName() << " id: " << getId()
         << " objSize: " << objSize << endl
         << flush;
#else
    fprintf(stderr,
            "GeneralPool<T,Alloc>::GeneralPool name: %s id: %d objSize: %d\n",
            getName(), getId(), objSize);
    fflush(stderr);
#endif
#endif
}

template <class T, class Alloc>
inline GeneralPool<T, Alloc>::GeneralPool(const char *name_,
                                          const AllocStrategy &strategy_)
    : GeneralPoolBase(0, name_), strategy(strategy_.copy()), counter(0),
      objSize(Alloc::objectSize()) {
    // set the rtti name
    const char *alloc_name = Alloc::getName();
    char comb_name[128];
    const type_info &ti = typeid(T);
    sprintf(comb_name, "%s::%s", alloc_name, ti.name());

    // set the rttiName in the base class
    setRttiName(comb_name);

#ifdef _MEM_STATS_
    // get the threshold for stack trace to kick in
    char *st = getenv("MEM_LEAK_THRESHOLD");
    unsigned long stack_th = 0;
    if (st) {
        stack_th = (unsigned long)atol(st);
    }
#endif

    for (unsigned int cnt = 0; cnt < MAX_POOL_BUCKETS; cnt++) {
        buckets[cnt].setBucketNum(cnt);
        buckets[cnt].makeStamp();
        buckets[cnt].setName(name);
        buckets[cnt].setRttiName(rttiName);

#ifdef _MEM_STATS_
        // buckets[cnt].setStackTraceFile(stackTraceFile);
        buckets[cnt].setStackTraceThreshold(stack_th);
#endif

        // execute the strategy
        buckets[cnt].setStrategy(*strategy);
    }

    // register w/PoolManager
    PoolManager &pm = PoolManager::getInstance();
    pm.registerPool(this);

#ifdef _MM_DEBUG_
#ifndef _MM_GMP_ON_
    cout << "GeneralPool<T,Alloc>::GeneralPool "
         << " name: " << getName() << " id: " << getId()
         << " objSize: " << objSize << endl
         << flush;
#else
    fprintf(stderr,
            "GeneralPool<T,Alloc>::GeneralPool name: %s id: %d objSize: %d\n",
            getName(), getId(), objSize);
    fflush(stderr);
#endif
#endif
}

template <class T, class Alloc> inline GeneralPool<T, Alloc>::~GeneralPool() {
    delete strategy;

    // unregister w/PoolManager
    PoolManager &pm = PoolManager::getInstance();
    pm.unregisterPool(this);
}

template <class T, class Alloc> inline void GeneralPool<T, Alloc>::manage() {
#ifdef _MM_DEBUG_
#ifndef _MM_GMP_ON_
    cout << "GeneralPool<T,Alloc>::manage "
         << " name: " << getName() << " id: " << getId() << endl
         << flush;
#else
    fprintf(stderr, "GeneralPool<T,Alloc>::manage name: %s id: %d\n", getName(),
            getId());
    fflush(stderr);
#endif
#endif
}

template <class T, class Alloc> inline T *GeneralPool<T, Alloc>::get() {
    // this has to be done in two steps to guarantee the right result in
    // multi-threaded environment
    unsigned int local_counter = counter++;

    // get the object from one of the buckets
    T *tmp_obj = buckets[local_counter % MAX_POOL_BUCKETS].get();

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
    cout << "GeneralPool<T,Alloc>::get, stamp: " << o_stamp
         << " bs: " << o_bucket_size << " bn: " << o_bucket_number
         << " objSize: " << objSize << " name: " << getName() << endl
         << flush;
#else
    fprintf(stderr,
            "GeneralPool<T,Alloc>::get, stamp: %d bs: %d bn: %d objSize: %d "
            "name: %s\n",
            o_stamp, o_bucket_size, o_bucket_number, objSize, getName());
    fflush(stderr);
#endif

#endif

    // return obj
    return tmp_obj;
}

template <class T, class Alloc>
inline T *GeneralPool<T, Alloc>::get(unsigned int pool_) {
    assert(pool_ < MAX_POOL_BUCKETS);

    // get the object from one of the buckets
    T *tmp_obj = buckets[pool_].getNoLock();

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
    cout << "GeneralPool<T,Alloc>::get, stamp: " << o_stamp
         << " bs: " << o_bucket_size << " bn: " << o_bucket_number
         << " objSize: " << objSize << " name: " << getName() << endl
         << flush;
#else
    fprintf(stderr,
            "GeneralPool<T,Alloc>::get, stamp: %d bs: %d bn: %d objSize: %d "
            "name: %s\n",
            o_stamp, o_bucket_size, o_bucket_number, objSize, getName());
    fflush(stderr);
#endif

#endif

    // return obj
    return tmp_obj;
}

template <class T, class Alloc>
inline void GeneralPool<T, Alloc>::put(T *obj_) {
#ifndef _MM_GMP_ON_
    unsigned int stamp = *(unsigned int *)((char *)obj_ + objSize);
#else
    unsigned int stamp = *(unsigned int *)((char *)obj_ - OBJ_PADDING);
#endif

    // compute bucket size and bucket number
    unsigned int bucket_size = stamp & BUCKET_SIZE_MASK;
    unsigned int bucket_number =
        (stamp >> BUCKET_NUMBER_SHIFT) & BUCKET_NUMBER_MASK;

#ifdef _MM_DEBUG_

#ifndef _MM_GMP_ON_
    cout << "GeneralPool<T,Alloc>::put, stamp: " << stamp
         << " bs: " << bucket_size << " bn: " << bucket_number
         << " objSize: " << objSize << " name: " << getName() << endl
         << flush;
#else
    fprintf(stderr,
            "GeneralPool<T,Alloc>::put, stamp: %d bs: %d bn: %d objSize: %d "
            "name: %s\n",
            stamp, bucket_size, bucket_number, objSize, getName());
    fflush(stderr);
#endif

#endif

    // check if the size and bucket number is right
    if (bucket_size != objSize) {
#ifndef _MM_GMP_ON_
        cerr << "Object returned to the wrong pool, obj size: " << bucket_size
             << " bucket size: " << objSize << " name: " << getName()
             << " id: " << getId() << " obj_: " << obj_ << endl
             << flush;
#else
        fprintf(stderr,
                "Object returned to the wrong pool, obj size: %d bucket size: "
                "%d name: %s id: %d obj_: %x\n",
                bucket_size, objSize, getName(), getId(), obj_);
        fflush(stderr);
#endif

        // dump all the pools and abort
        PoolManager &pm = PoolManager::getInstance();
        pm.dump();
        abort();
    }

    if (bucket_number >= MAX_POOL_BUCKETS) {
#ifndef _MM_GMP_ON_
        cerr << "Bucket number out of range, obj size " << bucket_size
             << " bucket size: " << objSize << " name: " << getName()
             << " id: " << getId() << " obj_: " << obj_ << endl
             << flush;
#else
        fprintf(stderr,
                "Bucket number out of range, obj size: %d bucket size: %d "
                "name: %s id: %d obj_: %x\n",
                bucket_size, objSize, getName(), getId(), obj_);
        fflush(stderr);
#endif

        // dump all the pools and abort
        PoolManager &pm = PoolManager::getInstance();
        pm.dump();
        abort();
    }

    // return it to the pool
    buckets[bucket_number].put(obj_);

    return;
}

template <class T, class Alloc>
inline void GeneralPool<T, Alloc>::put(T *obj_, unsigned int pool_) {
    assert(pool_ < MAX_POOL_BUCKETS);

#ifndef _MM_GMP_ON_
    unsigned int stamp = *(unsigned int *)((char *)obj_ + objSize);
#else
    unsigned int stamp = *(unsigned int *)((char *)obj_ - OBJ_PADDING);
#endif

    // compute bucket size and bucket number
    unsigned int bucket_size = stamp & BUCKET_SIZE_MASK;
    unsigned int bucket_number =
        (stamp >> BUCKET_NUMBER_SHIFT) & BUCKET_NUMBER_MASK;

#ifdef _MM_DEBUG_

#ifndef _MM_GMP_ON_
    cout << "GeneralPool<T,Alloc>::put, stamp: " << stamp
         << " bs: " << bucket_size << " bn: " << bucket_number
         << " objSize: " << objSize << " name: " << getName() << endl
         << flush;
#else
    fprintf(stderr,
            "GeneralPool<T,Alloc>::put, stamp: %d bs: %d bn: %d objSize: %d "
            "name: %s\n",
            stamp, bucket_size, bucket_number, objSize, getName());
    fflush(stderr);
#endif

#endif

    // check if the size and bucket number is right
    if (bucket_size != objSize) {
#ifndef _MM_GMP_ON_
        cerr << "Object returned to the wrong pool, obj size: " << bucket_size
             << " bucket size: " << objSize << " name: " << getName()
             << " id: " << getId() << " obj_: " << obj_ << endl
             << flush;
#else
        fprintf(stderr,
                "Object returned to the wrong pool, obj size: %d bucket size: "
                "%d name: %s id: %d obj_: %x\n",
                bucket_size, objSize, getName(), getId(), obj_);
        fflush(stderr);
#endif

        // dump all the pools and abort
        PoolManager &pm = PoolManager::getInstance();
        pm.dump();
        abort();
    }

    if (bucket_number >= MAX_POOL_BUCKETS) {
#ifndef _MM_GMP_ON_
        cerr << "Bucket number out of range, obj size " << bucket_size
             << " bucket size: " << objSize << " name: " << getName()
             << " id: " << getId() << " obj_: " << obj_ << endl
             << flush;
#else
        fprintf(stderr,
                "Bucket number out of range, obj size: %d bucket size: %d "
                "name: %s id: %d obj_: %x\n",
                bucket_size, objSize, getName(), getId(), obj_);
        fflush(stderr);
#endif

        // dump all the pools and abort
        PoolManager &pm = PoolManager::getInstance();
        pm.dump();
        abort();
    }

    // return it to the pool
    buckets[bucket_number].putNoLock(obj_);

    return;
}

template <class T, class Alloc> inline void GeneralPool<T, Alloc>::dump() {
#ifndef _MM_GMP_ON_
    cerr << "Dumping name: " << getName() << " id: " << getId()
         << " bucketSize: " << objSize << endl
         << flush;
#else
    fprintf(stderr, "Dumping name: %s id: %d bucketSize: %d\n", getName(),
            getId(), objSize);
    fflush(stderr);
#endif
    for (unsigned int cnt = 0; cnt < MAX_POOL_BUCKETS; cnt++) {
#ifndef _MM_GMP_ON_
        cerr << "-> Bucket no: " << cnt;
#else
        fprintf(stderr, "-> Bucket no: %d", cnt);
#endif
        buckets[cnt].dump();
    }
}

template <class T, class Alloc>
inline void GeneralPool<T, Alloc>::stats(FILE &file_) {
#ifdef _MEM_STATS_
    fprintf(&file_, " Pool: %s rttiName: %s id: %d obj_size: %d\n", getName(),
            getRttiName(), getId(), objSize);

    for (unsigned int cnt = 0; cnt < MAX_POOL_BUCKETS; cnt++) {
        buckets[cnt].stats(file_);
        fprintf(&file_, "\n");
    }

    fflush(&file_);
#endif
}

template <class T, class Alloc>
inline const AllocStrategy *GeneralPool<T, Alloc>::getStrategy() const {
    return strategy;
}

template <class T, class Alloc>
inline void GeneralPool<T, Alloc>::setStrategy(AllocStrategy *value) {
    strategy = value;
}
} // namespace MemMgmt
