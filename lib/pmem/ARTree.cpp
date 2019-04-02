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

#include "ARTree.h"
#include <Logger.h>
#include <algorithm>
#include <daqdb/Types.h>
#include <iostream>

namespace DaqDB {
#define LAYOUT "artree"

// Uncomment below to use PMDK allocation classes
#define USE_ALLOCATION_CLASSES 1

ARTree::ARTree(const string &_path, const size_t size,
               const size_t allocUnitSize) {
    tree = new TreeImpl(_path, size, allocUnitSize);
}

ARTree::~ARTree() {
    // TODO Auto-generated destructor stub
}

void TreeImpl::_initAllocClasses(const size_t allocUnitSize) {
    setClassId(ALLOC_CLASS_VALUE, allocUnitSize);
    setClassId(ALLOC_CLASS_VALUE_WRAPPER, sizeof(struct ValueWrapper));
    setClassId(ALLOC_CLASS_NODE256,
               NODE_SIZE[TYPE256] * sizeof(struct Node256));
    setClassId(ALLOC_CLASS_NODE_LEAF_COMPRESSED,
               NODE_SIZE[TYPE256] * sizeof(struct NodeLeafCompressed));
}

void TreeImpl::setClassId(enum ALLOC_CLASS c, size_t unit_size) {
    struct pobj_alloc_class_desc d = {};

    d.units_per_block = ALLOC_CLASS_UNITS_PER_BLOCK;
    d.alignment = ALLOC_CLASS_ALIGNMENT;
    d.header_type =
        (c == ALLOC_CLASS_VALUE) ? POBJ_HEADER_COMPACT : POBJ_HEADER_NONE;
    d.unit_size = unit_size;

    DAQ_DEBUG("ARTree alloc class (" + std::to_string(c) + ") unit_size=" +
              std::to_string(d.unit_size) + " units_per_block=" +
              std::to_string(d.units_per_block));
    int rc =
        pmemobj_ctl_set(_pm_pool.get_handle(), "heap.alloc_class.new.desc", &d);
    if (rc) {
        DAQ_DEBUG("ARTree alloc class init failed: " +
                  std::string(strerror(errno)) + ". Dumping existing classes");
        for (int i = 0; i < 255; i++) {
            d = {};
            std::string class_s =
                "heap.alloc_class." + std::to_string(i) + ".desc";
            rc = pmemobj_ctl_get(_pm_pool.get_handle(), class_s.c_str(), &d);
            if (rc)
                continue;
            DAQ_DEBUG("ARTree alloc class (" + std::to_string(d.class_id) +
                      ") unit_size=" + std::to_string(d.unit_size) +
                      " units_per_block=" + std::to_string(d.units_per_block));
            if (errno == EINVAL && d.unit_size == unit_size) {
                DAQ_DEBUG("Reusing exisiting alloc class");
                rc = 0;
                break;
            }
        }
        if (rc) {
            DAQ_CRITICAL("ARTree failed to allocate custom class");
            throw OperationFailedException(Status(ALLOCATION_ERROR));
        }
    }
    DAQ_DEBUG("ARTree alloc class defined, id= " + std::to_string(d.class_id));
    _allocClasses[c] = d.class_id;
}

unsigned TreeImpl::getClassId(enum ALLOC_CLASS c) { return _allocClasses[c]; }

TreeImpl::TreeImpl(const string &path, const size_t size,
                   const size_t allocUnitSize) {
    // Enforce performance options
    int enable = 1;
    int rc =
        pmemobj_ctl_set(_pm_pool.get_handle(), "prefault.at_create", &enable);
    if (rc)
        throw OperationFailedException(Status(UNKNOWN_ERROR));
    rc = pmemobj_ctl_set(_pm_pool.get_handle(), "prefault.at_open", &enable);
    if (rc)
        throw OperationFailedException(Status(UNKNOWN_ERROR));

    if (!boost::filesystem::exists(path)) {
        _pm_pool =
            pool<ARTreeRoot>::create(path, LAYOUT, size, S_IWUSR | S_IRUSR);
#ifdef USE_ALLOCATION_CLASSES
        _initAllocClasses(allocUnitSize);
#endif
        int depth = 0;
        int countNodes = 0;
        int levelsToAllocate = PREALLOC_LEVELS;
        treeRoot = _pm_pool.get_root().get();
        struct pobj_action _actionsArray[ACTION_NUMBER_NODE256];
        int _actionsCounter = 0;
#ifdef USE_ALLOCATION_CLASSES
        treeRoot->rootNode = pmemobj_xreserve(
            _pm_pool.get_handle(), &(_actionsArray[_actionsCounter++]),
            sizeof(Node256), VALUE,
            POBJ_CLASS_ID(getClassId(ALLOC_CLASS_NODE256)));
#else
        treeRoot->rootNode = pmemobj_reserve(
            _pm_pool.get_handle(), &(_actionsArray[_actionsCounter++]),
            sizeof(Node256), VALUE);
#endif

        if (OID_IS_NULL(*(treeRoot->rootNode).raw_ptr()))
            throw OperationFailedException(Status(ALLOCATION_ERROR));
        treeRoot->initialized = false;
        int status = pmemobj_publish(_pm_pool.get_handle(), _actionsArray,
                                     _actionsCounter);
        _actionsCounter = 0;
        treeRoot->rootNode->depth = 0;
        treeRoot->rootNode->type = TYPE256;
        DAQ_DEBUG("root creation");
        allocateFullLevels(treeRoot->rootNode, levelsToAllocate, _actionsArray,
                           _actionsCounter);
        status = pmemobj_publish(_pm_pool.get_handle(), _actionsArray,
                                 _actionsCounter);
        if (status != 0) {
            DAQ_CRITICAL("Error on publish = " + std::to_string(status));
        }
        _actionsCounter = 0;
        DAQ_DEBUG("root created");
    } else {
        _pm_pool = pool<ARTreeRoot>::open(path, LAYOUT);
#ifdef USE_ALLOCATION_CLASSES
        _initAllocClasses(allocUnitSize);
#endif
        treeRoot = _pm_pool.get_root().get();
        if (treeRoot)
            DAQ_DEBUG("Artree loaded");
        else
            std::cout << "Error on load" << std::endl;
    }
}

/*
 * Calculates tree size with recursive DFS algorithm.
 *
 * @param current pointer to a node from we wish to start counting.
 * @param leavesOnly determines if only leaves are counted.
 *
 */
uint64_t TreeImpl::getTreeSize(persistent_ptr<Node> current, bool leavesOnly) {
    persistent_ptr<Node256> node256;
    uint64_t size = 0;

    if (current->type == TYPE256) {
        // TYPE256
        node256 = current;
        for (int i = 0; i < NODE_SIZE[LEVEL_TYPES::TYPE256]; i++) {
            if (node256->children[i])
                size += getTreeSize(node256->children[i], leavesOnly);
        }

        if (!leavesOnly)
            size++;
    } else {
        // Count leaf
        size++;
    }

    return size;
}

uint8_t TreeImpl::getTreeDepth(persistent_ptr<Node> current) {
    persistent_ptr<Node256> node256;
    uint8_t depth = 0;

    if (current->type == TYPE256) {
        // TYPE256
        node256 = current;
        for (int i = 0; i < NODE_SIZE[LEVEL_TYPES::TYPE256]; i++) {
            if (node256->children[i])
                depth = std::max(getTreeDepth(node256->children[i]), depth);
        }
    }

    // Count this node
    depth++;

    return depth;
}

size_t ARTree::SetKeySize(size_t req_size) {
    tree->treeRoot->keySize =
        (sizeof(LEVEL_TYPE) / sizeof(int) - 1) * LEVEL_BYTES;
    /** @todo change to make it configurable at init time/
    if (!tree->treeRoot->initialized)
        tree->treeRoot->keySize = req_size;
    */
    return tree->treeRoot->keySize;
}

void ARTree::Get(const char *key, int32_t keybytes, void **value, size_t *size,
                 uint8_t *location) {
    Get(key, value, size, location);
}

void ARTree::Get(const char *key, void **value, size_t *size,
                 uint8_t *location) {
    persistent_ptr<ValueWrapper> valPrstPtr =
        tree->findValueInNode(tree->treeRoot->rootNode, key, false);
    if (valPrstPtr == nullptr)
        throw OperationFailedException(Status(KEY_NOT_FOUND));
    // std::cout << "Get off=" << (*valPrstPtr.raw_ptr()).off << std::endl;
    if (valPrstPtr->location == PMEM &&
        valPrstPtr->locationVolatile.get().value != EMPTY) {
        *value = valPrstPtr->locationPtr.value.get();
        *location = valPrstPtr->location;
    } else if (valPrstPtr->location == DISK) {
        *value = valPrstPtr->locationPtr.IOVptr.get();
        *location = valPrstPtr->location;
    } else {
        throw OperationFailedException(Status(KEY_NOT_FOUND));
    }
    *size = valPrstPtr->size;
}

uint64_t ARTree::GetTreeSize() {
    return tree->getTreeSize(tree->treeRoot->rootNode, false);
}

uint8_t ARTree::GetTreeDepth() {
    return tree->getTreeDepth(tree->treeRoot->rootNode);
}

uint64_t ARTree::GetLeafCount() {
    return tree->getTreeSize(tree->treeRoot->rootNode, true);
}

void ARTree::Put(const char *key, // copy value from std::string
                 char *value) {
    // printKey(key);
    persistent_ptr<ValueWrapper> valPrstPtr =
        tree->findValueInNode(tree->treeRoot->rootNode, key, false);
    if (valPrstPtr == nullptr)
        throw OperationFailedException(Status(KEY_NOT_FOUND));
    if (!tree->treeRoot->initialized) {
        tree->treeRoot->initialized = true;
    }
    valPrstPtr->locationVolatile.get().value = PMEM;
    valPrstPtr->location = PMEM;
}

void ARTree::Put(const char *key, int32_t keyBytes, const char *value,
                 int32_t valuebytes) {
    Put(key, nullptr);
}

/*
 * TODO: decrement value std::atomic<int> refCounter in parent and free it if
 * counter==0 When freeing - call decrementParent recurenty.
 * */
void ARTree::decrementParent(persistent_ptr<Node> node) {}

void ARTree::removeFromParent(persistent_ptr<ValueWrapper> valPrstPtr) {
    // @TODO: cleaning parent node not implemented
    decrementParent(valPrstPtr->parent);
}

void ARTree::Remove(const char *key) {
    persistent_ptr<ValueWrapper> valPrstPtr =
        tree->findValueInNode(tree->treeRoot->rootNode, key, false);

    if (valPrstPtr == nullptr || (valPrstPtr->location == EMPTY))
        throw OperationFailedException(Status(KEY_NOT_FOUND));

    try {
        removeFromParent(valPrstPtr);
        if (_isLocationReservedNotPublished(valPrstPtr)) {
            valPrstPtr->location = EMPTY;
            pmemobj_cancel(tree->_pm_pool.get_handle(), valPrstPtr->actionValue,
                           1);
            delete valPrstPtr->actionValue;
            pmemobj_free(valPrstPtr.raw_ptr());
        } else if (valPrstPtr->location == DISK) {
            // TODO: jradtke need to confirm if no extra action required here
            valPrstPtr->location = EMPTY;
        } else {
            // TODO: jradtke need to confirm if no extra action or error
            // handling required here
            valPrstPtr->location = EMPTY;
        }
    } catch (std::exception &e) {
        std::cout << "Error " << e.what();
        throw OperationFailedException(Status(UNKNOWN_ERROR));
    }
}

/*
 * Allocates tree levels
 *
 * @param node pointer to first node
 * @param levelsToAllocate how many levels below should be allocated
 *
 */
void TreeImpl::allocateFullLevels(persistent_ptr<Node> node,
                                  int levelsToAllocate,
                                  struct pobj_action *actionsArray,
                                  int &actionsCounter) {
    int depth = node->depth + 1; // depth of next level to be allocated
    persistent_ptr<Node256> node256;
    persistent_ptr<Node256> node256_new;
    persistent_ptr<NodeLeafCompressed> nodeLeafCompressed_new;
    persistent_ptr<Node> children[256];
    bool alloc_err = false;
    levelsToAllocate--;
    int i;

    if (node->type != TYPE256)
        return;

    node256 = node;

    if (LEVEL_TYPE[depth] == TYPE256) {
#ifdef USE_ALLOCATION_CLASSES
        node256_new = pmemobj_xreserve(
            _pm_pool.get_handle(), &(actionsArray[actionsCounter]),
            NODE_SIZE[node->type] * sizeof(Node256), VALUE,
            POBJ_CLASS_ID(getClassId(ALLOC_CLASS_NODE256)));
#else
        node256_new = pmemobj_reserve(
            _pm_pool.get_handle(), &(actionsArray[actionsCounter]),
            NODE_SIZE[node->type] * sizeof(Node256), VALUE);
#endif
        if (node256_new == nullptr) {
            DAQ_CRITICAL("reserve Node256 failed actionsCounter=" +
                         std::to_string(actionsCounter) + " with " +
                         std::string(strerror(errno)));
            throw OperationFailedException(Status(ALLOCATION_ERROR));
        }
        for (i = 0; i < NODE_SIZE[node->type]; i++) {
            node256_new->depth = depth;
            node256_new->type = TYPE256;
            node256_new->refCounter = 256; // TODO: check if it shouldn't be
                                           // incremented during PUT
            node256_new->parent = node;
            memset(node256_new->children, 0, sizeof(node256_new->children));
            node256->children[i] = node256_new;
            node256_new++;
        }
    } else if (LEVEL_TYPE[depth] == TYPE_LEAF_COMPRESSED) {
#ifdef USE_ALLOCATION_CLASSES
        nodeLeafCompressed_new = pmemobj_xreserve(
            _pm_pool.get_handle(), &(actionsArray[actionsCounter]),
            NODE_SIZE[node->type] * sizeof(NodeLeafCompressed), VALUE,
            POBJ_CLASS_ID(getClassId(ALLOC_CLASS_NODE_LEAF_COMPRESSED)));
#else
        nodeLeafCompressed_new = pmemobj_reserve(
            _pm_pool.get_handle(), &(actionsArray[actionsCounter]),
            NODE_SIZE[node->type] * sizeof(NodeLeafCompressed), VALUE);
#endif

        if (nodeLeafCompressed_new == nullptr) {
            DAQ_CRITICAL("reserve nodeLeafCompressed failed actionsCounter=" +
                         std::to_string(actionsCounter) + " with " +
                         std::string(strerror(errno)));
            throw OperationFailedException(Status(ALLOCATION_ERROR));
        }
        for (i = 0; i < NODE_SIZE[node->type]; i++) {
            nodeLeafCompressed_new->depth = depth;
            nodeLeafCompressed_new->type = TYPE_LEAF_COMPRESSED;
            nodeLeafCompressed_new->parent = node;
            nodeLeafCompressed_new->child = nullptr;
            node256->children[i] = nodeLeafCompressed_new;
            nodeLeafCompressed_new++;
        }
    }

    if (levelsToAllocate > 0) {
        /** @todo handle exceptions */
        allocateFullLevels(node256->children[i], levelsToAllocate, actionsArray,
                           actionsCounter);
    }
    actionsCounter = 1;
}

/*
 * Find value in Tree for a given key, allocate subtree if needed.
 *
 * @param current marks beggining of search
 * @param key pointer to searched key
 * @param allocate flag to specify if subtree should be allocated when key
 * not found
 * @return pointer to value
 */
persistent_ptr<ValueWrapper>
TreeImpl::findValueInNode(persistent_ptr<Node> current, const char *_key,
                          bool allocate) {
    size_t keyCalc;
    unsigned char *key = (unsigned char *)_key;
    persistent_ptr<Node256> node256;
    persistent_ptr<NodeLeafCompressed> nodeLeafCompressed;

    while (1) {
        DAQ_DEBUG("findValueInNode: current->depth= " +
                  std::to_string(current->depth));
        if (current->depth == ((sizeof(LEVEL_TYPE) / sizeof(int) - 1))) {
            // Node Compressed
            nodeLeafCompressed = current;
            if (nodeLeafCompressed->child == nullptr && allocate) {
                static thread_local struct pobj_action
                    actionsArray[ACTION_NUMBER_COMPRESSED];
                int actionsCounter = 0;

#ifdef USE_ALLOCATION_CLASSES
                persistent_ptr<ValueWrapper> valPrstPtr = pmemobj_xreserve(
                    _pm_pool.get_handle(), &(actionsArray[actionsCounter]),
                    sizeof(ValueWrapper), VALUE,
                    POBJ_CLASS_ID(getClassId(ALLOC_CLASS_VALUE_WRAPPER)));
#else
                persistent_ptr<ValueWrapper> valPrstPtr = pmemobj_reserve(
                    _pm_pool.get_handle(), &(actionsArray[actionsCounter]),
                    sizeof(ValueWrapper), VALUE);
#endif

                if (valPrstPtr == nullptr) {
                    DAQ_CRITICAL("reserve ValueWrapper failed with " +
                                 std::string(strerror(errno)));
                    throw OperationFailedException(Status(ALLOCATION_ERROR));
                }
                // std::cout << "valueWrapper off="
                //          << (*nodeLeafCompressed->child.raw_ptr()).off
                //          << std::endl;
                actionsCounter++;
                int status = pmemobj_publish(_pm_pool.get_handle(),
                                             actionsArray, actionsCounter);
                /** @todo add error check for all pmemobj_publish calls */
                actionsCounter = 0;
                /** @todo initialization below temporarily fixes race
                 * conditions when PUT and GET requests are executed in
                 * parallel on the same key. This is still not thread-safe.
                 */
                valPrstPtr->location = EMPTY;
                nodeLeafCompressed->child = valPrstPtr;
            }
            DAQ_DEBUG("findValueInNode: Found");
            return nodeLeafCompressed->child;
        }
        keyCalc = key[treeRoot->keySize - current->depth - 1];
        std::bitset<8> x(keyCalc);
        DAQ_DEBUG("findValueInNode: keyCalc=" + x.to_string());
        if (current->type == TYPE256) { // TYPE256
            node256 = current;
            if (!allocate && node256->children[keyCalc]) {
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
                    static thread_local struct pobj_action
                        actionsArray[ACTION_NUMBER_NODE256];
                    int actionsCounter = 0;
                    allocateFullLevels(node256, 1, actionsArray,
                                       actionsCounter);
                    int status = pmemobj_publish(_pm_pool.get_handle(),
                                                 actionsArray, actionsCounter);
                    actionsCounter = 0;
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

/*void ARTree::printKey(const char *key) {
    std::mutex localMutex;
    std::lock_guard<std::mutex> lock(localMutex);
    std::cout << "printing key" << std::endl;
    for (int i = 0; i < KEY_SIZE; i++) {
        std::cout << std::bitset<8>(key[KEY_SIZE - i - 1]) << std::endl;
    }
}*/

/*
 * Allocate value in ARTree for given key.
 *
 * @param key for which value should be allocated
 * @param size of value to be allocated
 * @param value stores reference where value pointer will be returned
 * @return StatusCode of operation
 */
void ARTree::AllocValueForKey(const char *key, size_t size, char **value) {
    int depth = 0;
    // printKey(key);
    if (!tree->treeRoot->initialized) {
        std::lock_guard<pmem::obj::mutex> lock(
            tree->treeRoot->rootNode->nodeMutex);
    }
    if (tree->treeRoot->rootNode) {
        persistent_ptr<ValueWrapper> valPrstPtr =
            tree->findValueInNode(tree->treeRoot->rootNode, key, true);
        if (valPrstPtr == nullptr || valPrstPtr->location != EMPTY)
            throw OperationFailedException(Status(ALLOCATION_ERROR));
        valPrstPtr->actionValue = new struct pobj_action;

#ifdef USE_ALLOCATION_CLASSES
        valPrstPtr->locationPtr.value = pmemobj_xreserve(
            tree->_pm_pool.get_handle(), valPrstPtr->actionValue, size, VALUE,
            POBJ_CLASS_ID(tree->getClassId(ALLOC_CLASS_VALUE)));
#else
        valPrstPtr->locationPtr.value = pmemobj_reserve(
            tree->_pm_pool.get_handle(), valPrstPtr->actionValue, size, VALUE);
#endif
        if (valPrstPtr->locationPtr.value == nullptr) {
            DAQ_CRITICAL("reserve Value failed with " +
                         std::string(strerror(errno)));
            throw OperationFailedException(Status(ALLOCATION_ERROR));
        }
        valPrstPtr->size = size;
        *value = valPrstPtr->locationPtr.value.get();
    } else {
        DAQ_DEBUG("root does not exist");
    }
}

/*
 * Allocate IOV Vector for given Key.
 * Vector is reserved and its address is returned.
 * Action related to reservation is stored in ValueWrapper in actionUpdate
 */
void ARTree::AllocateIOVForKey(const char *key, uint64_t **ptrIOV,
                               size_t size) {
    persistent_ptr<ValueWrapper> valPrstPtr;
    ValueWrapper *val;
    valPrstPtr = tree->findValueInNode(tree->treeRoot->rootNode, key, false);
    val =
        reinterpret_cast<ValueWrapper *>(pmemobj_direct(*valPrstPtr.raw_ptr()));
    val->actionUpdate = new pobj_action[3];
    /** @todo Consider changing to xreserve */
    pmemoid poid = pmemobj_reserve(tree->_pm_pool.get_handle(),
                                   &(val->actionUpdate[0]), size, IOV);
    if (OID_IS_NULL(poid)) {
        delete val->actionUpdate;
        val->actionUpdate = nullptr;
        throw OperationFailedException(Status(ALLOCATION_ERROR));
    }
    *ptrIOV = reinterpret_cast<uint64_t *>(pmemobj_direct(poid));
}

/*
 *	Updates location and locationPtr to STORAGE.
 *	Calls persist on IOVVector.
 *	Removes value buffer allocated in PMEM.
 */
void ARTree::UpdateValueWrapper(const char *key, uint64_t *ptr, size_t size) {
    pmemobj_persist(tree->_pm_pool.get_handle(), ptr, size);
    persistent_ptr<ValueWrapper> valPrstPtr;
    ValueWrapper *val;
    valPrstPtr = tree->findValueInNode(tree->treeRoot->rootNode, key, false);
    val =
        reinterpret_cast<ValueWrapper *>(pmemobj_direct(*valPrstPtr.raw_ptr()));
    pmemobj_set_value(tree->_pm_pool.get_handle(), &(val->actionUpdate[1]),
                      val->locationPtr.IOVptr.get(),
                      reinterpret_cast<uint64_t>(*ptr));
    pmemobj_set_value(tree->_pm_pool.get_handle(), &(val->actionUpdate[2]),
                      reinterpret_cast<uint64_t *>(&(val->location).get_rw()),
                      DISK);
    pmemobj_publish(tree->_pm_pool.get_handle(), val->actionUpdate, 3);
    pmemobj_cancel(tree->_pm_pool.get_handle(), val->actionValue, 1);
    delete[] val->actionUpdate;
    val->actionUpdate = nullptr;
}

} // namespace DaqDB
