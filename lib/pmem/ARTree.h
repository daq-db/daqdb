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

// LEVEL_TYPES and NODE SIZE are depenedent
enum LEVEL_TYPES { TYPE256, TYPE_LEAF_COMPRESSED };
const int NODE_SIZE[] = {256, 1};

// Describes Node type on each level of tree
const int LEVEL_TYPE[] = {TYPE256, TYPE256, TYPE256,
                          TYPE256, TYPE256, TYPE256,
                          TYPE256, TYPE256, TYPE_LEAF_COMPRESSED};
// how many levels will be created on ARTree initialization
const int PREALLOC_LEVELS = 1;

// Size of a single level in bytes
#define LEVEL_BYTES 1

// size of table for actions for each Node
#define ACTION_NUMBER_NODE256 (1 + 256)
#define ACTION_NUMBER_COMPRESSED 1

// Allocation class alignment
#define ALLOC_CLASS_ALIGNMENT 0
// Units per allocation block.
#define ALLOC_CLASS_UNITS_PER_BLOCK 100
enum ALLOC_CLASS {
    ALLOC_CLASS_VALUE,
    ALLOC_CLASS_VALUE_WRAPPER,
    ALLOC_CLASS_NODE256,
    ALLOC_CLASS_NODE_LEAF_COMPRESSED,
    ALLOC_CLASS_MAX
};

enum OBJECT_TYPES { VALUE, IOV };

struct locationWrapper {
    locationWrapper() : value(EMPTY) {}
    int value;
};
class Node;

struct ValueWrapper {
    explicit ValueWrapper() : actionValue(nullptr), actionUpdate(nullptr), location(EMPTY) {}
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
    persistent_ptr<Node> parent; // pointer to parent, needed for removal
};

class Node {
  public:
    explicit Node(int _depth, int _type) : depth(_depth), type(_type) {}
    // depth of current Node in tree
    int depth;
    // Type of Node: Node256 or compressed
    int type;
    persistent_ptr<Node> parent; // pointer to parent, needed for removal
    std::atomic<int> refCounter;
};

/*
 * Compressed node on last level in tree
 * Stores reference to ValueWrapper
 * */
class NodeLeafCompressed : public Node {
  public:
    explicit NodeLeafCompressed(int _depth, int _type) : Node(_depth, _type) {}
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
    bool initialized = false;
    size_t keySize; // bytes
};

class TreeImpl {
  public:
    TreeImpl(const string &path, const size_t size, const size_t allocUnitSize);
    void allocateFullLevels(persistent_ptr<Node> node, int levelsToAllocate,
                            struct pobj_action *actionsArray,
                            int &actionCounter);
    persistent_ptr<ValueWrapper> findValueInNode(persistent_ptr<Node> current,
                                                 const char *key,
                                                 bool allocate);
    ARTreeRoot *treeRoot;
    pool<ARTreeRoot> _pm_pool;
    void setClassId(enum ALLOC_CLASS c, size_t unit_size);
    unsigned getClassId(enum ALLOC_CLASS c);
    uint64_t getTreeSize(persistent_ptr<Node> current, bool leavesOnly = false);
    uint8_t getTreeDepth(persistent_ptr<Node> current);

  private:
    void _initAllocClasses(const size_t allocUnitSize);
    int _allocClasses[ALLOC_CLASS_MAX];
};

class ARTree : public DaqDB::RTreeEngine {
  public:
    ARTree(const string &path, const size_t size, const size_t allocUnitSize);
    virtual ~ARTree();
    string Engine() final { return "ARTree"; }
    size_t SetKeySize(size_t req_size);
    void Get(const char *key, int32_t keybytes, void **value, size_t *size,
             uint8_t *location) final;
    void Get(const char *key, void **value, size_t *size,
             uint8_t *location) final;
    uint64_t GetTreeSize() final;
    uint8_t GetTreeDepth() final;
    uint64_t GetLeafCount() final;
    void Put(const char *key, // copy value from std::string
             char *value) final;
    void Put(const char *key, int32_t keybytes, const char *value,
             int32_t valuebytes) final;
    void Remove(const char *key) final; // remove value for key
    void AllocValueForKey(const char *key, size_t size, char **value) final;
    void AllocateIOVForKey(const char *key, uint64_t **ptr, size_t size) final;
    void UpdateValueWrapper(const char *key, uint64_t *ptr, size_t size) final;
    void printKey(const char *key);
    void decrementParent(persistent_ptr<Node> node);
    void removeFromParent(persistent_ptr<ValueWrapper> valPrstPtr);

  private:
    inline bool
    _isLocationReservedNotPublished(persistent_ptr<ValueWrapper> valPrstPtr) {
        return (valPrstPtr->location == PMEM &&
                valPrstPtr->locationVolatile.get().value != EMPTY);
    }

    TreeImpl *tree;
};
} // namespace DaqDB
#endif /* LIB_STORE_ARTREE_H_ */
