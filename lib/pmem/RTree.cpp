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

#include "RTree.h"
#include <Logger.h>
#include <daqdb/Types.h>
#include <iostream>

namespace DaqDB {
#define LAYOUT "rtree"

RTree::RTree(const string &_path, const size_t size,
             const size_t allocUnitSize) {
    tree = new Tree(_path, size, allocUnitSize);
}

RTree::~RTree() {
    // TODO Auto-generated destructor stub
}
Tree::Tree(const string &path, const size_t size, const size_t allocUnitSize) {
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
            level_bits = treeRoot->level_bits;
            tree_heigh = treeRoot->tree_heigh;
            int count = 0;
            allocateLevel(treeRoot->rootNode, treeRoot->rootNode->depth,
                          &count);
        } catch (std::exception &e) {
            std::cout << "Error " << e.what();
        }

    } else {
        DAQ_DEBUG("RTree Opening existing pool");
        _pm_pool = pool<TreeRoot>::open(path, LAYOUT);
        treeRoot = _pm_pool.get_root().get();
        level_bits = treeRoot->level_bits;
        tree_heigh = treeRoot->tree_heigh;
    }

    // Define custom allocation class
    struct pobj_alloc_class_desc alloc_daqdb;
    alloc_daqdb.header_type = POBJ_HEADER_COMPACT;
    alloc_daqdb.unit_size = allocUnitSize;
    alloc_daqdb.units_per_block = ALLOC_CLASS_UNITS_PER_BLOCK;
    alloc_daqdb.alignment = ALLOC_CLASS_ALIGNMENT;
    int rc = pmemobj_ctl_set(_pm_pool.get_handle(), "heap.alloc_class.new.desc",
                             &alloc_daqdb);
    if (rc)
        throw OperationFailedException(Status(AllocationError));
    alloc_class = alloc_daqdb.class_id;
    DAQ_DEBUG("RTree New allocation class (" + std::to_string(alloc_class) +
              ") defined: unit_size=" + std::to_string(alloc_daqdb.unit_size) +
              " units_per_block=" +
              std::to_string(alloc_daqdb.units_per_block));
}

StatusCode RTree::Get(const char *key, int32_t keybytes, void **value,
                      size_t *size, uint8_t *location) {
    ValueWrapper *val = tree->findValueInNode(tree->treeRoot->rootNode, key);
    if (val->location == PMEM && val->locationVolatile.get().value != EMPTY) {
        *value = val->locationPtr.value.get();
        *location = val->location;
    } else if (val->location == DISK) {
        *value = val->locationPtr.IOVptr.get();
        *location = val->location;
    } else if (val->location == EMPTY ||
               val->locationVolatile.get().value == EMPTY) {
        return StatusCode::KeyNotFound;
    }
    *size = val->size;
    return StatusCode::Ok;
}

StatusCode RTree::Get(const char *key, void **value, size_t *size,
                      uint8_t *location) {
    ValueWrapper *val = tree->findValueInNode(tree->treeRoot->rootNode, key);
    if (val->location == PMEM && val->locationVolatile.get().value != EMPTY) {
        *value = val->locationPtr.value.get();
        *location = val->location;
    } else if (val->location == DISK) {
        *value = val->locationPtr.IOVptr.get();
        *location = val->location;
    } else if (val->location == EMPTY ||
               val->locationVolatile.get().value == EMPTY) {
        return StatusCode::KeyNotFound;
    }
    *size = val->size;
    return StatusCode::Ok;
}
StatusCode RTree::Put(const char *key, // copy value from std::string
                      char *value) {
    ValueWrapper *val = tree->findValueInNode(tree->treeRoot->rootNode, key);
    val->location = PMEM;
    val->locationVolatile.get().value = PMEM;
    return StatusCode::Ok;
}

StatusCode RTree::Put(const char *key, int32_t keybytes, const char *value,
                      int32_t valuebytes) {
    ValueWrapper *val = tree->findValueInNode(tree->treeRoot->rootNode, key);
    val->location = PMEM;
    val->locationVolatile.get().value = PMEM;
    return StatusCode::Ok;
}

StatusCode RTree::Remove(const char *key) {
    ValueWrapper *val = tree->findValueInNode(tree->treeRoot->rootNode, key);
    if (val->location == EMPTY) {
        return StatusCode::KeyNotFound;
    }
    try {
        if (val->location == PMEM &&
            val->locationVolatile.get().value != EMPTY) {
            pmemobj_cancel(tree->_pm_pool.get_handle(), val->actionValue, 1);
        } else if (val->location == DISK) {
            // @TODO jradtke need to confirm if no extra action required here
        }
        val->location = EMPTY;
    } catch (std::exception &e) {
        std::cout << "Error " << e.what();
        return StatusCode::UnknownError;
    }
    if (val->location == PMEM) {
        delete val->actionValue;
    }
    return StatusCode::Ok;
}

StatusCode RTree::AllocValueForKey(const char *key, size_t size, char **value) {
    ValueWrapper *val = tree->findValueInNode(tree->treeRoot->rootNode, key);
    val->actionValue = new pobj_action[1];
    pmemoid poid =
        pmemobj_xreserve(tree->_pm_pool.get_handle(), &(val->actionValue[0]),
                         size, VALUE, POBJ_CLASS_ID(tree->alloc_class));
    if (OID_IS_NULL(poid)) {
        delete val->actionValue;
        val->actionValue = nullptr;
        return StatusCode::AllocationError;
    }
    val->locationPtr.value = reinterpret_cast<char *>(pmemobj_direct(poid));
    val->size = size;
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
    val->actionUpdate = new pobj_action[3];
    pmemoid poid = pmemobj_reserve(tree->_pm_pool.get_handle(),
                                   &(val->actionUpdate[0]), size, IOV);
    if (OID_IS_NULL(poid)) {
        delete val->actionUpdate;
        val->actionUpdate = nullptr;
        return StatusCode::AllocationError;
    }
    *ptrIOV = reinterpret_cast<uint64_t *>(pmemobj_direct(poid));
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
                      val->locationPtr.IOVptr.get(),
                      reinterpret_cast<uint64_t>(*ptr));
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
        if (depth == (tree_heigh - 1)) {
            isLast = true;
        }
        if (depth == (tree_heigh - 1)) {
            (*count)++; // debug only
        }

        make_persistent_atomic<Node>(_pm_pool, current->children[i], isLast,
                                     depth);
        if (isLast) {
            make_persistent_atomic<ValueWrapper[]>(
                _pm_pool, current->children[i]->valueNode, LEVEL_SIZE);
        }
        if (depth < (tree_heigh - 1)) {
            allocateLevel(current->children[i], depth, count);
        }
    }
}

ValueWrapper *Tree::findValueInNode(persistent_ptr<Node> current,
                                    const char *key) {
    thread_local ValueWrapper *cachedVal = nullptr;
    thread_local char cachedKey[KEY_SIZE];
    int mask = ~(~0 << level_bits);
    ValueWrapper *val;
    int byteIndex;
    int positionInByte;
    int keyCalc;

    if (cachedVal && !memcmp(cachedKey, key, KEY_SIZE)) {
        return cachedVal;
    }

    while (1) {
        byteIndex = (current->depth * level_bits) / 8;
        positionInByte = (current->depth * level_bits) % 8;
        keyCalc = key[byteIndex] >> (8 - level_bits - positionInByte);
        keyCalc = keyCalc & mask;
        if (current->depth == (tree_heigh - 1)) {
            val = &(current->valueNode[keyCalc]);
            memcpy(cachedKey, key, KEY_SIZE);
            cachedVal = val;
            break;
        }
        current = current->children[keyCalc];
    };

    return val;
}
}
