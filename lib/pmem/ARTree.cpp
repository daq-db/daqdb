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

#include "ARTree.h"
#include <Logger.h>
#include <daqdb/Types.h>
#include <iostream>

namespace DaqDB {
#define LAYOUT "artree"

ARTree::ARTree(const string &_path, const size_t size,
               const size_t allocUnitSize) {
    tree = new TreeImpl(_path, size, allocUnitSize);
}

ARTree::~ARTree() {
    // TODO Auto-generated destructor stub
}
TreeImpl::TreeImpl(const string &path, const size_t size,
                   const size_t allocUnitSize) {
    if (!boost::filesystem::exists(path)) {
        try {
            _pm_pool =
                pool<ARTreeRoot>::create(path, LAYOUT, size, S_IWUSR | S_IRUSR);
        } catch (pmem::pool_error &pe) {
            std::cout << "Error on create" << pe.what();
        }
        try {
            int depth = 0;
            int countSlots = 0;
            int levelsToAllocate = PREALLOC_LEVELS;
            treeRoot = _pm_pool.get_root().get();
            make_persistent_atomic<Node4>(_pm_pool,
                                          treeRoot->rootNode, depth,
                                          LEVEL_TYPE[depth]);
            treeRoot->rootNode->counter = 0;
            allocateFullLevels(treeRoot->rootNode, depth, &countSlots, levelsToAllocate);
            //debug only
            countSlots = countSlots * LEVEL_TYPE[(sizeof(LEVEL_TYPE) / sizeof(int))-1];
            std::cout << "root created, count="<<countSlots << std::endl;
        } catch (std::exception &e) {
            std::cout << "Error " << e.what();
        }

    } else {
        _pm_pool = pool<ARTreeRoot>::open(path, LAYOUT);
        treeRoot = _pm_pool.get_root().get();
        std::cout << "Artree loaded" << std::endl;
    }
}

StatusCode ARTree::Get(const char *key, int32_t keybytes, void **value,
                       size_t *size, uint8_t *location) {

    return StatusCode::Ok;
}

StatusCode ARTree::Get(const char *key, void **value, size_t *size,
                       uint8_t *location) {

    return StatusCode::Ok;
}

StatusCode ARTree::Put(const char *key, // copy value from std::string
                       char *value) {
    return StatusCode::Ok;
}

StatusCode ARTree::Put(const char *key, int32_t keybytes, const char *value,
                       int32_t valuebytes) {
    return StatusCode::Ok;
}

StatusCode ARTree::Remove(const char *key) { return StatusCode::Ok; }

void TreeImpl::allocateFullLevels(persistent_ptr<Node> node, int depth, int *count, int levelsToAllocate) {
    depth++;
    levelsToAllocate--;
    persistent_ptr<Node4> node4;
    persistent_ptr<Node256> node256;
    if( levelsToAllocate > 0 ) {
        if (node->type == 4) {
            for (int i = 0; i < node->type; i++) {
                node4 = node;
                make_persistent_atomic<Node>(_pm_pool, node4->children[i],
                                             depth, LEVEL_TYPE[depth]);
                allocateFullLevels(node4->children[i], depth, count, levelsToAllocate);
            }
        } else {
            for (int i = 0; i < node->type; i++) {
                node256 = node;
                make_persistent_atomic<Node>(_pm_pool, node256->children[i],
                                             depth, LEVEL_TYPE[depth]);
                allocateFullLevels(node256->children[i], depth, count, levelsToAllocate);
            }
        }
    }
    if(levelsToAllocate==0) {
        (*count)++;
    }
}

StatusCode ARTree::AllocValueForKey(const char *key, size_t size,
                                    char **value) {
    int depth = 0;
    // int nodeType = 4;
    std::cout << "AllocValueForKey" << std::endl;
    if (tree->treeRoot->rootNode) {
        std::cout << "root exists" << std::endl;
    } else {
        std::cout << "root does not exist" << std::endl;
    }
    return StatusCode::Ok;
}

/*
 * Allocate IOV Vector for given Key.
 * Vector is reserved and it's address is returned.
 * Action related to reservation is stored in ValueWrapper in actionUpdate
 */
StatusCode ARTree::AllocateIOVForKey(const char *key, uint64_t **ptrIOV,
                                     size_t size) {

    return StatusCode::Ok;
}

/*
 *	Updates location and locationPtr to STORAGE.
 *	Calls persist on IOVVector.
 *	Removes value buffer allocated in PMEM.
 */
StatusCode ARTree::UpdateValueWrapper(const char *key, uint64_t *ptr,
                                      size_t size) {

    return StatusCode::Ok;
}

} // namespace DaqDB
