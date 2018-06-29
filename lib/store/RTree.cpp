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
    tree = new Tree(_path, size);
}

RTree::~RTree() {
    // TODO Auto-generated destructor stub
}
Tree::Tree(const string &path, const size_t size) {
    if (!boost::filesystem::exists(path)) {
        try {
            _pm_pool =
                pool<TreeRoot>::create(path, LAYOUT, size, S_IWUSR | S_IRUSR);
        } catch (pmem::pool_error &pe) {
            std::cout << "Error on create" << pe.what();
        }
        try {
            treeRoot = _pm_pool.get_root().get();
            make_persistent_atomic<Node>(_pm_pool, treeRoot->rootNode, false,
                                         0);
            treeRoot->level_bits = log2(LEVEL_SIZE);
            treeRoot->tree_heigh = KEY_SIZE / treeRoot->level_bits;
            int count = 0;
            allocateLevel(treeRoot->rootNode, treeRoot->rootNode->depth,
                          &count);
        } catch (std::exception &e) {
            std::cout << "Error " << e.what();
        }

    } else {
        std::cout << "Opening existing pool " << std::endl;
        _pm_pool = pool<TreeRoot>::open(path, LAYOUT);
        treeRoot = _pm_pool.get_root().get();
    }
}

StatusCode RTree::Get(const char *key, int32_t keybytes, char **value,
                      size_t *size) {
    ValueWrapper *val = tree->findValueInNode(tree->treeRoot->rootNode, key);
    if (val->location == EMPTY) {
        return StatusCode::KeyNotFound;
    } else if (val->location == PMEM) {
        *value = val->locationPtr.value.get();
    }
    *size = val->size;
    return StatusCode::Ok;
}

StatusCode RTree::Get(const char *key, char **value, size_t *size) {
    ValueWrapper *val = tree->findValueInNode(tree->treeRoot->rootNode, key);
    if (val->location == EMPTY) {
        return StatusCode::KeyNotFound;
    } else if (val->location == PMEM) {
        *value = val->locationPtr.value.get();
    }
    *size = val->size;
    return StatusCode::Ok;
}
StatusCode RTree::Put(const char *key, // copy value from std::string
                      char *value) {
    ValueWrapper *val = tree->findValueInNode(tree->treeRoot->rootNode, key);
    val->location = PMEM;
    return StatusCode::Ok;
}

StatusCode RTree::Put(const char *key, int32_t keybytes, const char *value,
                      int32_t valuebytes) {
    ValueWrapper *val = tree->findValueInNode(tree->treeRoot->rootNode, key);
    val->location = PMEM;
    return StatusCode::Ok;
}

StatusCode RTree::Remove(const char *key) {
    ValueWrapper *val = tree->findValueInNode(tree->treeRoot->rootNode, key);
    try {
        pmemobj_cancel(tree->_pm_pool.get_handle(), val->actionValue, 1);
        val->location = EMPTY;
    } catch (std::exception &e) {
        std::cout << "Error " << e.what();
        return StatusCode::UnknownError;
    }
    delete val->actionValue;
    return StatusCode::Ok;
}

StatusCode RTree::AllocValueForKey(const char *key, size_t size, char **value) {
    ValueWrapper *val = tree->findValueInNode(tree->treeRoot->rootNode, key);
    try {
        val->actionValue = new pobj_action[1];
        pmemoid poid;
        poid = pmemobj_reserve(tree->_pm_pool.get_handle(),
                               &(val->actionValue[0]), size, VALUE);
        val->locationPtr.value = reinterpret_cast<char *>(pmemobj_direct(poid));
        val->size = size;
    } catch (std::exception &e) {
        std::cout << "Error " << e.what();
        return StatusCode::UnknownError;
    }
    *value = reinterpret_cast<char *>(
        pmemobj_direct(*(val->locationPtr.value.raw_ptr())));
    return StatusCode::Ok;
}

/*
 * Allocate IOV Vector for given Key.
 * Vector is reserved and it's address is returned.
 * Action related to reservation is stored in ValueWrapper in actionUpdate
 */
StatusCode RTree::AllocateIOVForKey(const char *key, uint64_t **ptrIOV,
                                    size_t size) {
    ValueWrapper *val = tree->findValueInNode(tree->treeRoot->rootNode, key);
    try {
        val->actionUpdate = new pobj_action[3];
        pmemoid poid;
        poid = pmemobj_reserve(tree->_pm_pool.get_handle(),
                               &(val->actionUpdate[0]), size, IOV);
        *ptrIOV = reinterpret_cast<uint64_t *>(pmemobj_direct(poid));
    } catch (std::exception &e) {
        std::cout << "Error " << e.what();
        return StatusCode::UnknownError;
    }
    return StatusCode::Ok;
}
/*
 *	Updates location and locationPtr to STORAGE.
 *	Calls persist on IOVVector.
 *	Removes value buffer allocated in PMEM.
 */
StatusCode RTree::UpdateValueWrapper(const char *key, uint64_t *ptr,
                                     size_t size) {

    pmemobj_persist(tree->_pm_pool.get_handle(), ptr, size);
    ValueWrapper *val = tree->findValueInNode(tree->treeRoot->rootNode, key);
    pmemobj_set_value(tree->_pm_pool.get_handle(), &(val->actionUpdate[1]),
                      reinterpret_cast<uint64_t *>(pmemobj_direct(
                          *((val->locationPtr.IOVptr).raw_ptr()))),
                      reinterpret_cast<uint64_t>(ptr));
    pmemobj_set_value(tree->_pm_pool.get_handle(), &(val->actionUpdate[2]),
                      reinterpret_cast<uint64_t *>(&(val->location).get_rw()),
                      DISK);
    pmemobj_publish(tree->_pm_pool.get_handle(), val->actionUpdate, 3);
    pmemobj_cancel(tree->_pm_pool.get_handle(), val->actionValue, 1);
    delete val->actionValue;
    delete val->actionUpdate;
    return StatusCode::Ok;
}

void Tree::allocateLevel(persistent_ptr<Node> current, int depth, int *count) {
    int i;
    depth++;
    for (i = 0; i < LEVEL_SIZE; i++) {
        bool isLast = false;
        if (depth == (treeRoot->tree_heigh - 1)) {
            isLast = true;
        }
        if (depth == (treeRoot->tree_heigh - 1)) {
            (*count)++; // debug only
        }

        make_persistent_atomic<Node>(_pm_pool, current->children[i], isLast,
                                     depth);
        if (isLast) {
            make_persistent_atomic<ValueWrapper[]>(
                _pm_pool, current->children[i]->valueNode, LEVEL_SIZE);
        }
        if (depth < (treeRoot->tree_heigh - 1)) {
            allocateLevel(current->children[i], depth, count);
        }
    }
}

ValueWrapper *Tree::findValueInNode(persistent_ptr<Node> current,
                                    const char *key) {
    thread_local ValueWrapper *cachedVal = nullptr;
    thread_local char cachedKey[KEY_SIZE];
    int mask = ~(~0 << treeRoot->level_bits);
    ValueWrapper *val;
    int byteIndex;
    int positionInByte;
    int keyCalc;

    if (cachedVal && !memcmp(cachedKey, key, KEY_SIZE)) {
        return cachedVal;
    }

    while (1) {
        byteIndex = (current->depth * treeRoot->level_bits) / 8;
        positionInByte = (current->depth * treeRoot->level_bits) % 8;
        keyCalc = key[byteIndex] >> (8 - treeRoot->level_bits - positionInByte);
        keyCalc = keyCalc & mask;
        if (current->depth == (treeRoot->tree_heigh - 1)) {
            val = &(current->valueNode[keyCalc]);
            memcpy(cachedKey, key, KEY_SIZE);
            cachedVal = val;
            break;
        }
        current = current->children[keyCalc];
    };

    return val;
}
} // namespace FogKV
