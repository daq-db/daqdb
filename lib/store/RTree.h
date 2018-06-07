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

#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/make_persistent_array.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

#include <boost/filesystem.hpp>
#include <boost/filesystem/operations.hpp>

#include <climits>
#include <cmath>

using namespace pmem::obj;

// Number of keys slots inside Node (not key bits)
#define LEVEL_SIZE 4
// Number of key bits
#define KEY_SIZE 16

namespace FogKV {

struct ValueWrapper {
    p<int> location;
    persistent_ptr<char> value;
    struct pobj_action (* actionValue)[1];
    struct pobj_action (* actionKey)[2];
    string getString();
};

struct Node {
    explicit Node(bool _isEnd, int _depth) : isEnd(_isEnd), depth(_depth) {
        // children = make_persistent<Node[LEVEL_SIZE]>();
        if (isEnd) {
            valueNode = make_persistent<ValueWrapper[LEVEL_SIZE]>();
        }
    }
    persistent_ptr<Node> children[LEVEL_SIZE];
    bool isEnd;
    int depth;
    // persistent_ptr<int[LEVEL_SIZE]> values;		//only for leaves
    persistent_ptr<ValueWrapper[LEVEL_SIZE]> valueNode; // only for leaves
};

struct TreeRoot {
    persistent_ptr<Node> rootNode;
    p<int> level_bits;
    p<int> tree_heigh;
};

class Tree {
  public:
    Tree(const string &path, const size_t size);
    ValueWrapper *findValueInNode(persistent_ptr<Node> current, int key);
    void allocateLevel(persistent_ptr<Node> current, int depth, int *count);
    // persistent_ptr<Node> treeRoot;
    TreeRoot *treeRoot;
    pool<TreeRoot> _pm_pool;

  private:
};

class RTree : public FogKV::RTreeEngine {
  public:
    RTree(const string &path, const size_t size);
    virtual ~RTree();
    string Engine() final { return "RTree"; }
    StatusCode Get(int32_t limit, // copy value to fixed-size buffer
                   const char *key, int32_t keybytes, char *value,
                   int32_t *valuebytes) final;
    StatusCode Get(const string &key, // append value to std::string
                   string *value) final;
    StatusCode Put(const string &key, // copy value from std::string
                   const string &value) final;
    StatusCode Put(const char *key, int32_t keybytes, const char *value,
                   int32_t valuebytes) final;
    StatusCode Remove(const string &key) final; // remove value for key
    StatusCode AllocValueForKey(const string &key, size_t size,
                                char **value) final;

  private:
    Tree *tree;
};
}
#endif /* LIB_STORE_RTREE_H_ */
