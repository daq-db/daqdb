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
        _pm_pool =
            pool<TreeRoot>::create(path, LAYOUT, size, S_IWUSR | S_IRUSR);
        treeRoot = _pm_pool.get_root().get();
        make_persistent_atomic<Node>(_pm_pool, treeRoot->rootNode, false, 0);
        treeRoot->level_bits = log2(LEVEL_SIZE);
        treeRoot->tree_heigh = KEY_SIZE / treeRoot->level_bits;
        level_bits = treeRoot->level_bits;
        tree_heigh = treeRoot->tree_heigh;
        int count = 0;
        allocateLevel(treeRoot->rootNode, treeRoot->rootNode->depth, &count);
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
        throw OperationFailedException(Status(PMEM_ALLOCATION_ERROR));
    alloc_class = alloc_daqdb.class_id;
    DAQ_DEBUG(
        "RTree New allocation class (" + std::to_string(alloc_class) +
        ") defined: unit_size=" + std::to_string(alloc_daqdb.unit_size) +
        " units_per_block=" + std::to_string(alloc_daqdb.units_per_block));
}

size_t RTree::SetKeySize(size_t req_size) { return KEY_SIZE / BITS_IN_BYTE; }

void RTree::Get(const char *key, int32_t keybytes, void **value, size_t *size,
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
        throw OperationFailedException(Status(KEY_NOT_FOUND));
    }
    *size = val->size;
}

void RTree::Get(const char *key, void **value, size_t *size,
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
        throw OperationFailedException(Status(KEY_NOT_FOUND));
    }
    *size = val->size;
}

void RTree::Put(const char *key, // copy value from std::string
                char *value) {
    ValueWrapper *val = tree->findValueInNode(tree->treeRoot->rootNode, key);
    val->location = PMEM;
    val->locationVolatile.get().value = PMEM;
}

void RTree::Put(const char *key, int32_t keybytes, const char *value,
                int32_t valuebytes) {
    ValueWrapper *val = tree->findValueInNode(tree->treeRoot->rootNode, key);
    val->location = PMEM;
    val->locationVolatile.get().value = PMEM;
}

void RTree::Remove(const char *key) {
    ValueWrapper *val = tree->findValueInNode(tree->treeRoot->rootNode, key);
    if (val->location == EMPTY) {
        throw OperationFailedException(Status(KEY_NOT_FOUND));
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
        throw OperationFailedException(Status(UNKNOWN_ERROR));
    }
    if (val->location == PMEM) {
        delete val->actionValue;
    }
}

void RTree::AllocValueForKey(const char *key, size_t size, char **value) {
    ValueWrapper *val = tree->findValueInNode(tree->treeRoot->rootNode, key);
    val->actionValue = new pobj_action[1];
    pmemoid poid =
        pmemobj_xreserve(tree->_pm_pool.get_handle(), &(val->actionValue[0]),
                         size, VALUE, POBJ_CLASS_ID(tree->alloc_class));
    if (OID_IS_NULL(poid)) {
        delete val->actionValue;
        val->actionValue = nullptr;
        throw OperationFailedException(Status(PMEM_ALLOCATION_ERROR));
    }
    val->locationPtr.value = reinterpret_cast<char *>(pmemobj_direct(poid));
    val->size = size;
    *value = reinterpret_cast<char *>(
        pmemobj_direct(*(val->locationPtr.value.raw_ptr())));
}

/*
 * Allocate IOV Vector for given Key.
 * Vector is reserved and it's address is returned.
 * Action related to reservation is stored in ValueWrapper in actionUpdate
 */
void RTree::AllocateIOVForKey(const char *key, DeviceAddr **ptrIOV,
                              size_t size) {
    ValueWrapper *val = tree->findValueInNode(tree->treeRoot->rootNode, key);
    val->actionUpdate = new pobj_action[4];
    pmemoid poid = pmemobj_reserve(tree->_pm_pool.get_handle(),
                                   &(val->actionUpdate[0]), size, IOV);
    if (OID_IS_NULL(poid)) {
        delete val->actionUpdate;
        val->actionUpdate = nullptr;
        throw OperationFailedException(Status(PMEM_ALLOCATION_ERROR));
    }
    *ptrIOV = reinterpret_cast<DeviceAddr *>(pmemobj_direct(poid));
}

/*
 *	Updates location and locationPtr to STORAGE.
 *	Calls persist on IOVVector.
 *	Removes value buffer allocated in PMEM.
 */
void RTree::UpdateValueWrapper(const char *key, DeviceAddr *ptr, size_t size) {
    pmemobj_persist(tree->_pm_pool.get_handle(), ptr, size);
    ValueWrapper *val = tree->findValueInNode(tree->treeRoot->rootNode, key);
    pmemobj_set_value(tree->_pm_pool.get_handle(), &(val->actionUpdate[1]),
                      &val->locationPtr.IOVptr->lba, ptr->lba);
    pmemobj_set_value(tree->_pm_pool.get_handle(), &(val->actionUpdate[2]),
                      &val->locationPtr.IOVptr->busAddr.busAddr,
                      ptr->busAddr.busAddr);
    pmemobj_set_value(tree->_pm_pool.get_handle(), &(val->actionUpdate[3]),
                      reinterpret_cast<uint64_t *>(&(val->location).get_rw()),
                      DISK);
    pmemobj_publish(tree->_pm_pool.get_handle(), val->actionUpdate, 4);
    pmemobj_cancel(tree->_pm_pool.get_handle(), val->actionValue, 1);
    delete val->actionValue;
    delete val->actionUpdate;
}

/*
 * Allocate IOV Vector for given Key and Update the Wrapper.
 * Vector is reserved and its address is returned.
 * Action related to reservation is stored in ValueWrapper in actionUpdate
 */
void RTree::AllocateAndUpdateValueWrapper(const char *key, size_t size,
                                          const DeviceAddr *devAddr) {
    persistent_ptr<ValueWrapper> valPrstPtr;
    ValueWrapper *val;
    valPrstPtr = tree->findValueInNode(tree->treeRoot->rootNode, key);
    struct pobj_action actions[4];
    val =
        reinterpret_cast<ValueWrapper *>(pmemobj_direct(*valPrstPtr.raw_ptr()));
    val->actionUpdate = actions;
    /** @todo Consider changing to xreserve */
    pmemoid poid = pmemobj_reserve(tree->_pm_pool.get_handle(),
                                   val->actionUpdate, size, IOV);
    if (OID_IS_NULL(poid)) {
        delete val->actionUpdate;
        val->actionUpdate = nullptr;
        throw OperationFailedException(Status(PMEM_ALLOCATION_ERROR));
    }

    DeviceAddr *ptr = reinterpret_cast<DeviceAddr *>(pmemobj_direct(poid));
    pmemobj_persist(tree->_pm_pool.get_handle(), ptr, size);
    val =
        reinterpret_cast<ValueWrapper *>(pmemobj_direct(*valPrstPtr.raw_ptr()));
    pmemobj_set_value(tree->_pm_pool.get_handle(), &(val->actionUpdate[1]),
                      &val->locationPtr.IOVptr->lba, devAddr->lba);
    pmemobj_set_value(tree->_pm_pool.get_handle(), &(val->actionUpdate[2]),
                      &val->locationPtr.IOVptr->busAddr.busAddr,
                      devAddr->busAddr.busAddr);
    pmemobj_set_value(tree->_pm_pool.get_handle(), &(val->actionUpdate[3]),
                      reinterpret_cast<uint64_t *>(&(val->location).get_rw()),
                      DISK);
    pmemobj_publish(tree->_pm_pool.get_handle(), val->actionUpdate, 4);
    pmemobj_cancel(tree->_pm_pool.get_handle(), val->actionValue, 1);

    val->actionUpdate = nullptr;
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
} // namespace DaqDB
