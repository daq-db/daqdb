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
// The maximum number of memory blocks to use.
// The size of a single block will be:
//      TOTAL_PMEM_SIZE / (MIN_VALUE_SIZE * MAXIMUM_MEMORY_BLOCKS)
#define MAXIMUM_MEMORY_BLOCKS 16
// Allocation class alignment
#define ALLOC_CLASS_ALIGNMENT 0

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
        persistent_ptr<uint64_t> IOVptr;
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
    StatusCode Get(const char *key, int32_t keybytes, void **value,
                   size_t *size, uint8_t *location) final;
    StatusCode Get(const char *key, void **value, size_t *size,
                   uint8_t *location) final;
    StatusCode Put(const char *key, // copy value from std::string
                   char *value) final;
    StatusCode Put(const char *key, int32_t keybytes, const char *value,
                   int32_t valuebytes) final;
    StatusCode Remove(const char *key) final; // remove value for key
    StatusCode AllocValueForKey(const char *key, size_t size,
                                char **value) final;
    StatusCode AllocateIOVForKey(const char *key, uint64_t **ptr,
                                 size_t size) final;
    StatusCode UpdateValueWrapper(const char *key, uint64_t *ptr,
                                  size_t size) final;

  private:
    Tree *tree;
};
}
#endif /* LIB_STORE_RTREE_H_ */
