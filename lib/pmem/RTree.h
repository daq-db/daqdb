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

#ifndef LIB_STORE_RTREE_H_
#define LIB_STORE_RTREE_H_

#include "RTreeEngine.h"

#include <libpmemobj++/experimental/v.hpp>
#include <libpmemobj++/make_persistent_array_atomic.hpp>
#include <libpmemobj++/make_persistent_atomic.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>
#include <libpmemobj++/utils.hpp>

#include <boost/filesystem.hpp>
#include <boost/filesystem/operations.hpp>

#include <climits>
#include <cmath>
#include <iostream>

using namespace pmem::obj::experimental;
using namespace pmem::obj;

// Number of keys slots inside Node (not key bits)
#define LEVEL_SIZE 4
// Number of key bits
#define KEY_SIZE 24 /** @TODO jschmieg: target value is 24 bits */
// Allocation class alignment
#define ALLOC_CLASS_ALIGNMENT 0
// Units per allocation block.
#define ALLOC_CLASS_UNITS_PER_BLOCK 1000
#define BITS_IN_BYTE 8

enum OBJECT_TYPES { VALUE, IOV };

namespace DaqDB {

struct locationStruct {
    locationStruct() : value(EMPTY) {}
    int value;
};

struct ValueWrapper {
    explicit ValueWrapper()
        : actionValue(nullptr), actionUpdate(nullptr), location(EMPTY) {}
    p<int> location;
    union locationPtr {
        persistent_ptr<char> value; // for location == PMEM
        persistent_ptr<DeviceAddr> IOVptr;
        locationPtr() : value(nullptr){};
    } locationPtr;

    p<size_t> size;
    v<locationStruct> locationVolatile;
    struct pobj_action *actionValue;
    struct pobj_action *actionUpdate;
    string getString();
};

struct Node {
    explicit Node(bool _isEnd, int _depth) : isEnd(_isEnd), depth(_depth) {}
    persistent_ptr<Node> children[LEVEL_SIZE];
    bool isEnd;
    int depth;
    persistent_ptr<ValueWrapper[]> valueNode; // only for leaves
};

struct TreeRoot {
    persistent_ptr<Node> rootNode;
    p<int> level_bits;
    p<int> tree_heigh;
};

class Tree {
  public:
    Tree(const string &path, const size_t size, const size_t allocUnitSize);
    ValueWrapper *findValueInNode(persistent_ptr<Node> current,
                                  const char *key);
    void allocateLevel(persistent_ptr<Node> current, int depth, int *count);
    TreeRoot *treeRoot;
    pool<TreeRoot> _pm_pool;
    int level_bits;
    int tree_heigh;
    int alloc_class;

  private:
};

class RTree : public DaqDB::RTreeEngine {
  public:
    RTree(const string &path, const size_t size, const size_t allocUnitSize);
    virtual ~RTree();
    string Engine() final { return "RTree"; }
    size_t SetKeySize(size_t req_size);
    void Get(const char *key, int32_t keybytes, void **value, size_t *size,
             uint8_t *location) final;
    void Get(const char *key, void **value, size_t *size,
             uint8_t *location) final;
    void Put(const char *key, // copy value from std::string
             char *value) final;
    void Put(const char *key, int32_t keybytes, const char *value,
             int32_t valuebytes) final;
    void Remove(const char *key) final; // remove value for key
    void AllocValueForKey(const char *key, size_t size, char **value) final;
    void AllocateIOVForKey(const char *key, DeviceAddr **ptr,
                           size_t size) final;
    void UpdateValueWrapper(const char *key, DeviceAddr *ptr,
                            size_t size) final;

  private:
    Tree *tree;
};
}
#endif /* LIB_STORE_RTREE_H_ */
