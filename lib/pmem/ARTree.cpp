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
            int countNodes = 0;
            int levelsToAllocate = PREALLOC_LEVELS;
            treeRoot = _pm_pool.get_root().get();
            _actionCounter = 0;
            treeRoot->rootNode = pmemobj_reserve(
                _pm_pool.get_handle(), &(_actionsArray[_actionCounter++]),
                sizeof(Node256), VALUE);
            int status = pmemobj_publish(_pm_pool.get_handle(), _actionsArray,
                                         _actionCounter);
            _actionCounter = 0;
            treeRoot->rootNode->actionsArray = (struct pobj_action *)malloc(
                ACTION_NUMBER * sizeof(struct pobj_action));
            treeRoot->rootNode->actionCounter = 0;
            treeRoot->rootNode->depth = 0;
            treeRoot->rootNode->type = TYPE256;
            DAQ_DEBUG("root creation");
            allocateFullLevels(treeRoot->rootNode, levelsToAllocate);
            status = pmemobj_publish(_pm_pool.get_handle(),
                                     treeRoot->rootNode->actionsArray,
                                     treeRoot->rootNode->actionCounter);
            free(treeRoot->rootNode->actionsArray);
            treeRoot->rootNode->actionsArray = nullptr;
            if (status != 0) {
                DAQ_DEBUG("Error on publish = " + std::to_string(status));
            }
            treeRoot->rootNode->actionCounter = 0;
            DAQ_DEBUG("root created");
        } catch (std::exception &e) {
            std::cout << "Error " << e.what();
        }

    } else {
        _pm_pool = pool<ARTreeRoot>::open(path, LAYOUT);
        treeRoot = _pm_pool.get_root().get();
        if(treeRoot)
            DAQ_DEBUG("Artree loaded");
        else
            std::cout << "Error on load" << std::endl;
    }
}

StatusCode ARTree::Get(const char *key, int32_t keybytes, void **value,
                       size_t *size, uint8_t *location) {
    ValueWrapper *val =
        tree->findValueInNode(tree->treeRoot->rootNode, key, false);
    if (!val)
        return StatusCode::KEY_NOT_FOUND;
    if (val->location == PMEM && val->locationVolatile.get().value != EMPTY) {
        *value = val->locationPtr.value.get();
        *location = val->location;
    } else if (val->location == DISK) {
        *value = val->locationPtr.IOVptr.get();
        *location = val->location;
    } else if (val->location == EMPTY ||
               val->locationVolatile.get().value == EMPTY) {
        return StatusCode::KEY_NOT_FOUND;
    }
    *size = val->size;
    return StatusCode::OK;
}

StatusCode ARTree::Get(const char *key, void **value, size_t *size,
                       uint8_t *location) {
    ValueWrapper *val =
        tree->findValueInNode(tree->treeRoot->rootNode, key, false);
    if (!val)
        return StatusCode::KEY_NOT_FOUND;
    if (val->location == PMEM && val->locationVolatile.get().value != EMPTY) {
        *value = val->locationPtr.value.get();
        *location = val->location;
    } else if (val->location == DISK) {
        *value = val->locationPtr.IOVptr.get();
        *location = val->location;
    } else if (val->location == EMPTY ||
               val->locationVolatile.get().value == EMPTY) {
        return StatusCode::KEY_NOT_FOUND;
    }
    *size = val->size;
    return StatusCode::OK;
}

StatusCode ARTree::Put(const char *key, // copy value from std::string
                       char *value) {
    ValueWrapper *val =
        tree->findValueInNode(tree->treeRoot->rootNode, key, false);
    val->location = PMEM;
    val->locationVolatile.get().value = PMEM;
    return StatusCode::OK;
}

StatusCode ARTree::Put(const char *key, int32_t keyBytes, const char *value,
                       int32_t valuebytes) {
    ValueWrapper *val =
        tree->findValueInNode(tree->treeRoot->rootNode, key, false);
    val->location = PMEM;
    val->locationVolatile.get().value = PMEM;
    return StatusCode::OK;
}

StatusCode ARTree::Remove(const char *key) {

    ValueWrapper *val = tree->findValueInNode(tree->treeRoot->rootNode, key, false);
    if (!val) {
        return StatusCode::KEY_NOT_FOUND;
    }
    if (val->location == EMPTY) {
        return StatusCode::KEY_NOT_FOUND;
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
        return StatusCode::UNKNOWN_ERROR;
    }
    if (val->location == PMEM) {
        delete val->actionValue;
    }
    return StatusCode::OK;
}

/*
 * Allocates tree levels
 *
 * @param node pointer to first node
 * @param levelsToAllocate how many levels below should be allocated
 *
 */
void TreeImpl::allocateFullLevels(persistent_ptr<Node> node,
                                  int levelsToAllocate) {
    int depth = node->depth + 1; // depth of next level to be allocated
    persistent_ptr<Node256> node256;
    persistent_ptr<Node256> node256_new;
    levelsToAllocate--;

    if (node->type == TYPE256) {
        for (int i = 0; i < NODE_SIZE[node->type]; i++) {
            node256 = node;
            if (LEVEL_TYPE[depth] == TYPE256) {
                    node256_new= pmemobj_reserve(
                    _pm_pool.get_handle(),
                    &(node256->actionsArray[node256->actionCounter++]),
                    sizeof(Node256), VALUE);
                if (OID_IS_NULL(*(node256_new).raw_ptr())) {
                    DAQ_DEBUG("reserve failed node256->actionCounter=" +
                              std::to_string(node256->actionCounter));
                }
                node256_new->actionsArray =
                    (struct pobj_action *)malloc(ACTION_NUMBER *
                                                 sizeof(struct pobj_action));
                node256_new->actionCounter = 0;
                node256_new->depth = depth;
                node256_new->type = TYPE256;
                node256->children[i] = node256_new;
            } else if (LEVEL_TYPE[depth] == TYPE_LEAF_COMPRESSED) {
                node256->children[i] = pmemobj_reserve(
                    _pm_pool.get_handle(),
                    &(node256->actionsArray[node256->actionCounter++]),
                    sizeof(NodeLeafCompressed), VALUE);
                node256->children[i]->depth = depth;
                node256->children[i]->type = TYPE_LEAF_COMPRESSED;
            }

            if (levelsToAllocate > 0) {
                allocateFullLevels(node256->children[i],
                                   levelsToAllocate);
            }
        }
    }
}

/*
 * Find value in Tree for a given key, allocate subtree if needed.
 *
 * @param current marks beggining of search
 * @param key pointer to searched key
 * @param allocate flag to specify if subtree should be allocated when key not
 * found
 * @return pointer to value
 */
ValueWrapper *TreeImpl::findValueInNode(persistent_ptr<Node> current,
                                        const char *_key, bool allocate) {
    size_t keyCalc;
    unsigned char *key = (unsigned char *)_key;
    int debugCount = 0;
    persistent_ptr<Node256> node256;
    persistent_ptr<NodeLeafCompressed> nodeLeafCompressed;
    ValueWrapper *val;

    while (1) {
        keyCalc = key[current->depth];
        std::bitset<8> x(keyCalc);
        DAQ_DEBUG("findValueInNode: current->depth= " +
                 std::to_string(current->depth) + " keyCalc=" + x.to_string());
        if (current->depth == ((sizeof(LEVEL_TYPE) / sizeof(int) - 1))) {
            // Node Compressed
            nodeLeafCompressed = current;
            if (allocate) {
                nodeLeafCompressed->key = keyCalc;
                nodeLeafCompressed->child = pmemobj_reserve(
                    _pm_pool.get_handle(), &(_actionsArray[_actionCounter++]),
                    sizeof(ValueWrapper), VALUE);
                int status = pmemobj_publish(_pm_pool.get_handle(),
                                             _actionsArray, _actionCounter);
                _actionCounter = 0;
                val = reinterpret_cast<ValueWrapper *>(
                    (nodeLeafCompressed->child).raw_ptr());
                return val;
            } else {
                DAQ_DEBUG("findValueInNode: not allocate, keyCalc=" +
                          std::to_string(keyCalc) + "NodeLeafCompressed->key=" +
                          std::to_string(nodeLeafCompressed->key));
                if (nodeLeafCompressed->key == keyCalc) {
                    val = reinterpret_cast<ValueWrapper *>(
                        (nodeLeafCompressed->child).raw_ptr());
                    DAQ_DEBUG("findValueInNode: Found");
                    return val;
                } else {
                    DAQ_DEBUG("findValueInNode: Not Found");
                    return nullptr;
                }
            }
        }
        if (current->type == TYPE256) { // TYPE256
            node256 = current;
            if (node256->children[keyCalc]) {
                current = node256->children[keyCalc];
            } else if (allocate) {
                std::lock_guard<pmem::obj::mutex> lock(node256->nodeMutex);
                // other threads don't have to create subtree
                if (node256->children[keyCalc]) {
                    current = node256->children[keyCalc];
                } else { // not found, allocate subtree
                    DAQ_DEBUG("findValueInNode: allocate subtree on depth=" +
                              std::to_string(node256->depth + 1) + " type=" +
                              std::to_string(LEVEL_TYPE[node256->depth + 1]));
                    allocateFullLevels(node256, 1);
                    int status = pmemobj_publish(_pm_pool.get_handle(),
                                                 node256->actionsArray,
                                                 node256->actionCounter);
                    node256->actionCounter = 0;
                    free(node256->actionsArray);
                    node256->actionsArray = nullptr;
                    current = node256->children[keyCalc];
                }
            } else {
                // not found, do not allocate subtree
                DAQ_DEBUG("findValueInNode: Not Found");
                return nullptr;
            }
        }
    }
    DAQ_DEBUG("findValueInNode: Not Found");
    return nullptr;
}
/*
 * Allocate value in ARTree for given key.
 *
 * @param key for which value should be allocated
 * @param size of value to be allocated
 * @param value stores reference where value pointer will be returned
 * @return StatusCode of operation
 */
StatusCode ARTree::AllocValueForKey(const char *key, size_t size,
                                    char **value) {
    int depth = 0;
    if (tree->treeRoot->rootNode) {
        ValueWrapper *val =
            tree->findValueInNode(tree->treeRoot->rootNode, key, true);
        val->actionValue = new pobj_action[1];
        pmemoid poid = pmemobj_reserve(tree->_pm_pool.get_handle(),
                                       &(val->actionValue[0]), size, VALUE);
        if (OID_IS_NULL(poid)) {
            delete val->actionValue;
            val->actionValue = nullptr;
            return StatusCode::ALLOCATION_ERROR;
        }
        val->locationPtr.value = reinterpret_cast<char *>(pmemobj_direct(poid));
        val->size = size;
        *value = reinterpret_cast<char *>(
            pmemobj_direct(*(val->locationPtr.value.raw_ptr())));
    } else {
        DAQ_DEBUG("root does not exist");
    }
    return StatusCode::OK;
}

/*
 * Allocate IOV Vector for given Key.
 * Vector is reserved and its address is returned.
 * Action related to reservation is stored in ValueWrapper in actionUpdate
 */
StatusCode ARTree::AllocateIOVForKey(const char *key, uint64_t **ptrIOV,
                                     size_t size) {
    ValueWrapper *val =
        tree->findValueInNode(tree->treeRoot->rootNode, key, false);
    val->actionUpdate = new pobj_action[3];
    pmemoid poid = pmemobj_reserve(tree->_pm_pool.get_handle(),
                                   &(val->actionUpdate[0]), size, IOV);
    if (OID_IS_NULL(poid)) {
        delete val->actionUpdate;
        val->actionUpdate = nullptr;
        return StatusCode::ALLOCATION_ERROR;
    }
    *ptrIOV = reinterpret_cast<uint64_t *>(pmemobj_direct(poid));
    return StatusCode::OK;
}

/*
 *	Updates location and locationPtr to STORAGE.
 *	Calls persist on IOVVector.
 *	Removes value buffer allocated in PMEM.
 */
StatusCode ARTree::UpdateValueWrapper(const char *key, uint64_t *ptr,
                                      size_t size) {
    pmemobj_persist(tree->_pm_pool.get_handle(), ptr, size);
    ValueWrapper *val =
        tree->findValueInNode(tree->treeRoot->rootNode, key, false);
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
    return StatusCode::OK;
}

} // namespace DaqDB
