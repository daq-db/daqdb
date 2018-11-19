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

#ifndef LIB_STORE_ARTREE_H_
#define LIB_STORE_ARTREE_H_

#include "RTreeEngine.h"

#include <libpmemobj++/experimental/v.hpp>
#include <libpmemobj++/make_persistent_array_atomic.hpp>
#include <libpmemobj++/make_persistent_atomic.hpp>
#include <libpmemobj++/mutex.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/shared_mutex.hpp>
#include <libpmemobj++/transaction.hpp>
#include <libpmemobj++/utils.hpp>

#include <boost/filesystem.hpp>
#include <boost/filesystem/operations.hpp>

#include <atomic>
#include <bitset>
#include <climits>
#include <cmath>
#include <iostream>
#include <mutex>

using namespace pmem::obj::experimental;
using namespace pmem::obj;
namespace DaqDB {

//LEVEL_TYPES and NODE SIZE are depenedent
enum LEVEL_TYPES { TYPE256, TYPE_LEAF_COMPRESSED };
const int NODE_SIZE[] = { 256, 1};

// Describes Node type on each level of tree
const int LEVEL_TYPE[] = {TYPE256, TYPE256, TYPE256, TYPE256,   TYPE256,
                          TYPE256, TYPE256, TYPE256, TYPE_LEAF_COMPRESSED};
// how many levels will be created on ARTree initialization
const int PREALLOC_LEVELS = 1;
//size of table for actions for each Node
#define ACTION_NUMBER (1 + 256)

enum OBJECT_TYPES { VALUE, IOV };

struct locationWrapper {
    locationWrapper() : value(EMPTY) {}
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
    v<locationWrapper> locationVolatile;
    struct pobj_action *actionValue;
    struct pobj_action *actionUpdate;
};

class Node {
  public:
    explicit Node(int _depth, int _type)
        : depth(_depth), type(_type), actionCounter(0) {}
    //depth of current Node in tree
    int depth;
    //Type of Node: Node256 or compressed
    int type;
    //stores all actions done on subtree with Reserve-publish
    struct pobj_action *actionsArray;
    //counts all actions done on subtree with Reserve-publish
    int actionCounter;
};

/*
 * Compressed node on last level in tree
 * Stores reference to ValueWrapper
 * */
class NodeLeafCompressed : public Node {
  public:
    explicit NodeLeafCompressed(int _depth, int _type) : Node(_depth, _type) {}
    uint32_t key;
    persistent_ptr<ValueWrapper> child; // pointer to Value
};

class Node256 : public Node {
  public:
    explicit Node256(int _depth, int _type) : Node(_depth, _type) {}
    persistent_ptr<Node> children[256]; // array of pointers to Nodes
    pmem::obj::mutex nodeMutex;
};

struct ARTreeRoot {
    persistent_ptr<Node256> rootNode;
    pmem::obj::mutex mutex;
    p<int> level_bits;
};

class TreeImpl {
  public:
    TreeImpl(const string &path, const size_t size, const size_t allocUnitSize);
    void allocateFullLevels(persistent_ptr<Node> node, int levelsToAllocate);
    ValueWrapper *findValueInNode(persistent_ptr<Node> current, const char *key,
                                  bool allocate);
    ARTreeRoot *treeRoot;
    pool<ARTreeRoot> _pm_pool;
  private:
    struct pobj_action _actionsArray[ACTION_NUMBER];
    int _actionCounter;
};

class ARTree : public DaqDB::RTreeEngine {
  public:
    ARTree(const string &path, const size_t size, const size_t allocUnitSize);
    virtual ~ARTree();
    string Engine() final { return "ARTree"; }
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
    TreeImpl *tree;
};
} // namespace DaqDB
#endif /* LIB_STORE_ARTREE_H_ */
