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

#include "RTree.h"
#include <iostream>

namespace FogKV {
#define LAYOUT "rtree"

RTree::RTree(const string &_path, const size_t size) {
    std::cout << "Creating RTree " << std::endl;
    tree = new Tree(_path, size);
}

RTree::~RTree() {
    // TODO Auto-generated destructor stub
}
Tree::Tree(const string &path, const size_t size) {
    if (!boost::filesystem::exists(path)) {
        std::cout << "Creating pool " << std::endl;
        try {
            _pm_pool =
                pool<TreeRoot>::create(path, LAYOUT, size, S_IWUSR | S_IRUSR);
        } catch (pmem::pool_error &pe) {
            std::cout << "Error on create" << pe.what();
        }
        try {
            transaction::exec_tx(_pm_pool, [&] {
                treeRoot = _pm_pool.get_root().get();
                treeRoot->rootNode = make_persistent<Node>(false, 0);
                treeRoot->level_bits = log2(LEVEL_SIZE);
                treeRoot->tree_heigh = KEY_SIZE / treeRoot->level_bits;
                int count = 0;
                allocateLevel(treeRoot->rootNode, treeRoot->rootNode->depth,
                              &count);
            });
        } catch (std::exception &e) {
            std::cout << "Error " << e.what();
        }

    } else {
        std::cout << "Opening existing pool " << std::endl;
        _pm_pool = pool<TreeRoot>::open(path, LAYOUT);
        treeRoot = _pm_pool.get_root().get();
    }

    std::cout << "Tree constructor " << std::endl;
}

StatusCode RTree::Get(const char *key, int32_t keybytes, string *value) {
    ValueWrapper *val =
        tree->findValueInNode(tree->treeRoot->rootNode, atoi(key));
    value->append(val->value.get());

    return StatusCode::Ok;
}

StatusCode RTree::Get(const string &key, // append value to std::string
                      string *value) {
    ValueWrapper *val =
        tree->findValueInNode(tree->treeRoot->rootNode, atoi(key.c_str()));
    value->append(val->value.get());

    return StatusCode::Ok;
}
StatusCode RTree::Put(const string &key, // copy value from std::string
                      const string &value) {
    ValueWrapper *val =
        tree->findValueInNode(tree->treeRoot->rootNode, atoi(key.c_str()));
    val->location = 1;
    return StatusCode::Ok;
}

StatusCode RTree::Put(const char *key, int32_t keybytes, const char *value,
                      int32_t valuebytes) {
    ValueWrapper *val =
        tree->findValueInNode(tree->treeRoot->rootNode, atoi(key));
    val->location = 1;
    return StatusCode::Ok;
}

StatusCode RTree::Remove(const string &key) { return StatusCode::Ok; }

StatusCode RTree::AllocValueForKey(const string &key, size_t size,
                                   char **value) {
    ValueWrapper *val =
        tree->findValueInNode(tree->treeRoot->rootNode, atoi(key.c_str()));
    try {
        pmemobj_zalloc(tree->_pm_pool.get_handle(), val->value.raw_ptr(), size,
                       1);
    } catch (std::exception &e) {
        std::cout << "Error " << e.what();
        return StatusCode::UnknownError;
    }
    *value = reinterpret_cast<char *>(pmemobj_direct(*(val->value.raw_ptr())));
    return StatusCode::Ok;
}

void Tree::allocateLevel(persistent_ptr<Node> current, int depth, int *count) {
    int i;
    depth++;

    for (i = 0; i < LEVEL_SIZE; i++) {
        bool isLast = false;
        if (depth == ((treeRoot->tree_heigh) - 1)) {
            isLast = true;
        }
        if (depth == treeRoot->tree_heigh) {
            (*count)++; // debug only
        }

        current->children[i] = make_persistent<Node>(isLast, depth);

        if (depth < treeRoot->tree_heigh) {
            allocateLevel(current->children[i], depth, count);
        }
    }
}

ValueWrapper *Tree::findValueInNode(persistent_ptr<Node> current, int key) {
    int key_shift = (2 * (treeRoot->tree_heigh - current->depth - 1));
    int key_calc = key >> key_shift;
    int mask = ~(UINT_MAX << treeRoot->level_bits);
    key_calc = key_calc & mask;

    if (current->depth != (treeRoot->tree_heigh - 1)) {
        return findValueInNode(current->children[key_calc], key);
    }
    return &(current->valueNode[key_calc]);
}
}
