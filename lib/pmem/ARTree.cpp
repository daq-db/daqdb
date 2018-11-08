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
            allocateFullLevels(treeRoot->rootNode, &countNodes,
                               levelsToAllocate);
            DAQ_DEBUG("Publishing, actionCounter=" +
                      std::to_string(treeRoot->rootNode->actionCounter));
            status = pmemobj_publish(_pm_pool.get_handle(),
                                     treeRoot->rootNode->actionsArray,
                                     treeRoot->rootNode->actionCounter);
            free(treeRoot->rootNode->actionsArray);
            treeRoot->rootNode->actionsArray = nullptr;
            if (status == 0) {
                DAQ_DEBUG("Published");
            } else {
                DAQ_DEBUG("Error on publish = " + std::to_string(status));
            }
            treeRoot->rootNode->actionCounter = 0;
            // debug only
            // countSlots =
            //    countSlots * LEVEL_TYPE[depth + levelsToAllocate -1];
            DAQ_DEBUG("root created, count=" + std::to_string(countNodes));
        } catch (std::exception &e) {
            std::cout << "Error " << e.what();
        }

    } else {
        _pm_pool = pool<ARTreeRoot>::open(path, LAYOUT);
        treeRoot = _pm_pool.get_root().get();
        DAQ_DEBUG("Artree loaded");
    }
}

StatusCode ARTree::Get(const char *key, int32_t keybytes, void **value,
                       size_t *size, uint8_t *location) {
    ValueWrapper *val =
        tree->findValueInNode(tree->treeRoot->rootNode, key, false);
    if (!val)
        return StatusCode::KeyNotFound;
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

StatusCode ARTree::Get(const char *key, void **value, size_t *size,
                       uint8_t *location) {

    ValueWrapper *val =
        tree->findValueInNode(tree->treeRoot->rootNode, key, false);
    if (!val)
        return StatusCode::KeyNotFound;
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

StatusCode ARTree::Put(const char *key, // copy value from std::string
                       char *value) {
    DAQ_DEBUG("Put1 ");
    ValueWrapper *val =
        tree->findValueInNode(tree->treeRoot->rootNode, key, false);
    val->location = PMEM;
    val->locationVolatile.get().value = PMEM;
    return StatusCode::Ok;
}

StatusCode ARTree::Put(const char *key, int32_t keyBytes, const char *value,
                       int32_t valuebytes) {
    DAQ_DEBUG("Put2");
    ValueWrapper *val =
        tree->findValueInNode(tree->treeRoot->rootNode, key, false);
    val->location = PMEM;
    val->locationVolatile.get().value = PMEM;
    return StatusCode::Ok;
}

StatusCode ARTree::Remove(const char *key) {
    ValueWrapper *val = tree->findValueInNode(tree->treeRoot->rootNode, key, false);
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

/*
 * Allocates tree levels
 *
 * @param node pointer to first node
 * @param count debug counter
 * @param levelsToAllocate how many levels below should be allocated
 *
 */
void TreeImpl::allocateFullLevels(persistent_ptr<Node> node, int *count,
                                  int levelsToAllocate) {
    int depth = node->depth + 1; // depth of next level to be allocated
    levelsToAllocate--;
    if (node->actionsArray)
        DAQ_DEBUG("2. node->actionsArray OK, depth=" + std::to_string(depth));
    else
        DAQ_DEBUG("2. node->actionsArray NOK, depth=" + std::to_string(depth));

    // DAQ_DEBUG("allocateFullLevels, depth=" + std::to_string(depth) + "
    // levelsToAllocate=" + std::to_string(levelsToAllocate) + " count="
    // +std::to_string(*count));
    persistent_ptr<Node4> node4;
    persistent_ptr<Node256> node256;
    // if (levelsToAllocate > 0) {
    if (node->type == TYPE4) {
        /*for (int i = 0; i < NODE_SIZE[node->type]; i++) {
            node4 = node;
            if (LEVEL_TYPE[depth] == TYPE4) {
                // DAQ_DEBUG("Reserve 4, actionCounter="
                // +std::to_string(actionCounter)+ " size Node4=" +
                // std::to_string(sizeof(Node4)));
                node4->children[i] = pmemobj_reserve(
                    _pm_pool.get_handle(), &(actionsArray[actionCounter++]),
                    sizeof(Node4), VALUE);
                // if (OID_IS_NULL(poid)) {
                //    DAQ_DEBUG("reserve failed");
                //}
                // node256->children[i]= static_cast<Node4
                // *>(pmemobj_direct(poid));
                node4->children[i]->depth = depth;
                node4->children[i]->type = TYPE4;
            } else if (LEVEL_TYPE[depth] == TYPE256) {
                // DAQ_DEBUG("Reserve 256, actionCounter="
                // +std::to_string(actionCounter) + " size Node256=" +
                // std::to_string(sizeof(Node256)));
                node4->children[i] = pmemobj_reserve(
                    _pm_pool.get_handle(), &(actionsArray[actionCounter++]),
                    sizeof(Node256), VALUE);
                // if (OID_IS_NULL(poid)) {
                //    DAQ_DEBUG("reserve failed");
                //}
                // node256->children[i]= static_cast<Node256
                // *>(pmemobj_direct(poid));

                node4->children[i]->depth = depth;
                node4->children[i]->type = TYPE256;

            }
        }*/
    } else if (node->type == TYPE256) {
        for (int i = 0; i < NODE_SIZE[node->type]; i++) {
            node256 = node;
            if (node->actionsArray)
                DAQ_DEBUG("3. node->actionsArray OK, depth=" +
                          std::to_string(depth));
            else
                DAQ_DEBUG("3. node->actionsArray NOK, depth=" +
                          std::to_string(depth));
            if (node256->actionsArray)
                DAQ_DEBUG("3. node256->actionsArray OK, depth=" +
                          std::to_string(depth));
            else
                DAQ_DEBUG("3. node256->actionsArray NOK, depth=" +
                          std::to_string(depth));
            if (LEVEL_TYPE[depth] == TYPE4) {
                /*DAQ_DEBUG("Reserve 4, actionCounter="
                +std::to_string(actionCounter)+ " size Node4=" +
                std::to_string(sizeof(Node4)));*/
                node256->children[i] = pmemobj_reserve(
                    _pm_pool.get_handle(),
                    &(node256->actionsArray[node256->actionCounter++]),
                    sizeof(Node4), VALUE);
                // if (OID_IS_NULL(poid)) {
                //    DAQ_DEBUG("reserve failed");
                //}
                // node256->children[i]= static_cast<Node4
                // *>(pmemobj_direct(poid));
                node256->children[i]->depth = depth;
                node256->children[i]->type = TYPE4;
            } else if (LEVEL_TYPE[depth] == TYPE256) {
                // DAQ_DEBUG("Reserve 256, actionCounter="
                // +std::to_string(actionCounter) + " size Node256=" +
                // std::to_string(sizeof(Node256)));
                if (node256->actionsArray)
                    DAQ_DEBUG("node256->actionsArray OK, depth=" +
                              std::to_string(depth) +
                              " node256->actionCounter=" +
                              std::to_string(node256->actionCounter));
                else
                    DAQ_DEBUG("node256->actionsArray NOK, depth=" +
                              std::to_string(depth) +
                              " node256->actionCounter=" +
                              std::to_string(node256->actionCounter));
                node256->children[i] = pmemobj_reserve(
                    _pm_pool.get_handle(),
                    &(node256->actionsArray[node256->actionCounter++]),
                    sizeof(Node256), VALUE);
                if (OID_IS_NULL(*(node256->children[i]).raw_ptr())) {
                    DAQ_DEBUG("reserve failed node256->actionCounter=" +
                              std::to_string(node256->actionCounter));
                }
                DAQ_DEBUG(
                    "allocate: " +
                    std::to_string(ACTION_NUMBER * sizeof(struct pobj_action)));
                node256->children[i]->actionsArray =
                    (struct pobj_action *)malloc(ACTION_NUMBER *
                                                 sizeof(struct pobj_action));
                node256->children[i]->actionCounter = 0;
                // std::cout << "alloc" << std::endl;
                if (node256->children[i]->actionsArray)
                    DAQ_DEBUG(
                        "node256->children[" + std::to_string(i) +
                        "]->actionsArray OK, depth=" + std::to_string(depth));
                else
                    DAQ_DEBUG(
                        "node256->children[" + std::to_string(i) +
                        "]->actionsArray NOK, depth=" + std::to_string(depth));
                // node256->children[i]= static_cast<Node256
                // *>(pmemobj_direct(poid));

                node256->children[i]->depth = depth;
                node256->children[i]->type = TYPE256;
                /*make_persistent_atomic<Node256>(_pm_pool,
                   node256->children[i], depth, LEVEL_TYPE[depth]);*/
            } else if (LEVEL_TYPE[depth] == TYPE4_LEAF) {
                /*DAQ_DEBUG("Reserve 4 leaf, actionCounter="
                 +std::to_string(actionCounter) + " size Node4leaf=" +
                 std::to_string(sizeof(Node4Leaf)));*/
                node256->children[i] = pmemobj_reserve(
                    _pm_pool.get_handle(),
                    &(node256->actionsArray[node256->actionCounter++]),
                    sizeof(Node4Leaf), VALUE);
                node256->children[i]->depth = depth;
                node256->children[i]->type = TYPE4_LEAF;
            }

            // make_persistent_atomic<Node256>(_pm_pool, node256->children[i],
            //                             depth, LEVEL_TYPE[depth]);
            // DAQ_DEBUG("allocateFullLevelsNode256, count="
            // +std::to_string(*count));
            (*count)++;

            if (levelsToAllocate > 0) {
                allocateFullLevels(node256->children[i], count,
                                   levelsToAllocate);
            }
        }
    }

    //}
    /*if (levelsToAllocate == 0) {
        (*count)++;
    }*/
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
    persistent_ptr<Node4> node4;
    persistent_ptr<Node256> node256;
    persistent_ptr<Node4Leaf> node4Leaf;
    persistent_ptr<NodeLeafCompressed> nodeLeafCompressed;
    ValueWrapper *val;

    while (1) {
        keyCalc = key[current->depth];
        std::bitset<8> x(keyCalc);
        DAQ_DEBUG("findValueInNode: current->depth= " +
                  std::to_string(current->depth) + " keyCalc=" + x.to_string());
        // std::cout << "findValueInNode" << "depth=" << current->depth << "x="
        // << x << std::endl;
        /*if (current->depth == ((sizeof(LEVEL_TYPE) / sizeof(int) - 1))) {
            //last level can only be Node4Leaf
            node4Leaf = current;
            if (allocate) {
                node4Leaf->keys[0] = keyCalc;
                node4Leaf->children[0] = pmemobj_reserve(
                    _pm_pool.get_handle(), &(actionsArray[actionCounter++]),
                    sizeof(ValueWrapper), VALUE);
                int status = pmemobj_publish(_pm_pool.get_handle(),
                                             actionsArray, actionCounter);
                actionCounter = 0;
                val = reinterpret_cast<ValueWrapper *>(
                    (node4Leaf->children[0]).rakeyCalcw_ptr());
                return val;
            } else {
                DAQ_DEBUG("findValueInNode: not allocate, keyCalc=" +
                          std::to_string(keyCalc) + "node4Leaf->keys[0]=" +
                          std::to_string(node4Leaf->keys[0]));
                if (node4Leaf->keys[0] == keyCalc) {
                    val = reinterpret_cast<ValueWrapper *>(
                        (node4Leaf->children[0]).raw_ptr());
                    DAQ_DEBUG("findValueInNode: Found");
                    return val;
                    // current = node4->children[i];
                } else {
                    return nullptr;
                }
            }
        }*/

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
                /*DAQ_DEBUG("findValueInNode: not allocate, keyCalc=" +
                          std::to_string(keyCalc) + "NodeLeafCompressed->key=" +
                          std::to_string(nodeLeafCompressed->key));*/
                if (nodeLeafCompressed->key == keyCalc) {
                    val = reinterpret_cast<ValueWrapper *>(
                        (nodeLeafCompressed->child).raw_ptr());
                    DAQ_DEBUG("findValueInNode: Found");
                    return val;
                    // current = node4->children[i];
                } else {
                    return nullptr;
                }
            }
        }
        if (current->type == TYPE4) {
            DAQ_DEBUG("findValueInNode: Type4 on depth=" +
                      std::to_string(current->depth));
            /*node4 = current;
            int i = 0;
            for (i = 0; i < 4; i++) {
                DAQ_DEBUG("checking node4, i = " + std::to_string(i));
                if (node4->keys[i] == keyCalc) {
                    // Node4 should count number of elements but this requires
                    // atomic add To be sure that correct key was found we need
                    // to check if value ptr exists It is only observed when
                    // keyCalc == 0 and keys array is not initialized
                    if (node4->children[i]) {
                        DAQ_DEBUG("found correct key");
                        current = node4->children[i];
                        break;
                    } else {
                        DAQ_DEBUG("Found empty key");
                        i = 4;
                        break;
                    }
                }
            }
            if (i == 4) {
                DAQ_DEBUG("not found");
                break;
            }
        */
        } else if (current->type == TYPE256) { // TYPE256
            node256 = current;
            if (node256->children[keyCalc]) {
                current = node256->children[keyCalc];
            } else if (allocate) {
                std::lock_guard<pmem::obj::mutex> lock(node256->nodeMutex);
                DAQ_DEBUG(
                    "locked "); //+ std::to_string(&(node256->nodeMutex)));
                // other threads don't have to create subtree
                if (node256->children[keyCalc]) {
                    DAQ_DEBUG("findValueInNode: other thread meets allocated "
                              "subtree");
                    current = node256->children[keyCalc];
                } else { // not found, allocate subtree
                    DAQ_DEBUG("findValueInNode: allocate subtree on depth=" +
                              std::to_string(node256->depth + 1) + " type=" +
                              std::to_string(LEVEL_TYPE[node256->depth + 1]));
                    allocateFullLevels(node256, &debugCount, 1);
                    int status = pmemobj_publish(_pm_pool.get_handle(),
                                                 node256->actionsArray,
                                                 node256->actionCounter);
                    node256->actionCounter = 0;
                    free(node256->actionsArray);
                    node256->actionsArray = nullptr;
                    /*if (status == 0) {
                        DAQ_DEBUG("Published, debugCount=" +
                                  std::to_string(debugCount));
                    }*/
                    debugCount = 0;
                    current = node256->children[keyCalc];
                }
                DAQ_DEBUG("unlocked ");
            } else {
                // not found, do not allocate subtree
                return nullptr;
            }
        }
        /*else if (current->type == TYPE4_LEAF) {
            node4Leaf = current;
            if (node4Leaf->keys[0] == keyCalc) {
                current = node4Leaf->children[0];
            }
        }*/
        // current = current->children[keyCalc];
    }

    return nullptr;
}

StatusCode ARTree::AllocValueForKey(const char *key, size_t size,
                                    char **value) {
    int depth = 0;
    DAQ_DEBUG("<-----AllocValueForKey , key=" + std::to_string(*key));
    if (tree->treeRoot->rootNode) {
        DAQ_DEBUG("root exists");
        ValueWrapper *val =
            tree->findValueInNode(tree->treeRoot->rootNode, key, true);
        val->actionValue = new pobj_action[1];
        pmemoid poid = pmemobj_reserve(tree->_pm_pool.get_handle(),
                                       &(val->actionValue[0]), size, VALUE);
        if (OID_IS_NULL(poid)) {
            delete val->actionValue;
            val->actionValue = nullptr;
            return StatusCode::AllocationError;
        }
        val->locationPtr.value = reinterpret_cast<char *>(pmemobj_direct(poid));
        val->size = size;
        *value = reinterpret_cast<char *>(
            pmemobj_direct(*(val->locationPtr.value.raw_ptr())));
    } else {
        DAQ_DEBUG("root does not exist");
    }
    return StatusCode::Ok;
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
    return StatusCode::Ok;
}

} // namespace DaqDB
