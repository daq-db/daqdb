/**
 * Copyright 2018, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of the copyright holder nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef LIB_STORE_RTREE_H_
#define LIB_STORE_RTREE_H_

#include "RTreeEngine.h"

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

using namespace pmem::obj;

// Number of keys slots inside Node (not key bits)
#define LEVEL_SIZE 4
// Number of key bits
#define KEY_SIZE 24 /** @TODO jschmieg: target value is 24 bits */

enum OBJECT_TYPES { VALUE, IOV };

enum LOCATIONS { EMPTY, PMEM, DISK };

namespace FogKV {

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
    struct pobj_action *actionValue;
    struct pobj_action *actionUpdate;
    string getString();
};

struct Node {
    explicit Node(bool _isEnd, int _depth) : isEnd(_isEnd), depth(_depth) {
    }
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
    Tree(const string &path, const size_t size);
    ValueWrapper *findValueInNode(persistent_ptr<Node> current,
                                  const char *key);
    void allocateLevel(persistent_ptr<Node> current, int depth, int *count);
    TreeRoot *treeRoot;
    pool<TreeRoot> _pm_pool;

  private:
};

class RTree : public FogKV::RTreeEngine {
  public:
    RTree(const string &path, const size_t size);
    virtual ~RTree();
    string Engine() final { return "RTree"; }
    StatusCode Get(const char *key, int32_t keybytes, char **value,
                   size_t *size) final;
    StatusCode Get(const char *key, char **value, size_t *size) final;
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
} // namespace FogKV
#endif /* LIB_STORE_RTREE_H_ */
