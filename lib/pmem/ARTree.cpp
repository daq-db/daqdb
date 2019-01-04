/**
 * Copyright 2018 - 2019 Intel Corporation.
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

void TreeImpl::_initAllocClasses(const size_t allocUnitSize) {
    struct pobj_alloc_class_desc alloc_daqdb;
    alloc_daqdb.units_per_block = ALLOC_CLASS_UNITS_PER_BLOCK;
    alloc_daqdb.alignment = ALLOC_CLASS_ALIGNMENT;

    // value
    alloc_daqdb.header_type = POBJ_HEADER_COMPACT;
    alloc_daqdb.unit_size = allocUnitSize;
    int rc = pmemobj_ctl_set(_pm_pool.get_handle(), "heap.alloc_class.new.desc",
                             &alloc_daqdb);
    if (rc)
        throw OperationFailedException(Status(ALLOCATION_ERROR));
    setClassId(ALLOC_CLASS_VALUE, alloc_daqdb.class_id);
    DAQ_DEBUG("ARTree alloc class (value) (" +
              std::to_string(alloc_daqdb.class_id) + ") defined: unit_size=" +
              std::to_string(alloc_daqdb.unit_size) + " units_per_block=" +
              std::to_string(alloc_daqdb.units_per_block));

    // value wrapper
    alloc_daqdb.header_type = POBJ_HEADER_NONE;
    alloc_daqdb.unit_size = sizeof(struct ValueWrapper);
    rc = pmemobj_ctl_set(_pm_pool.get_handle(), "heap.alloc_class.new.desc",
                         &alloc_daqdb);
    if (rc)
        throw OperationFailedException(Status(ALLOCATION_ERROR));
    setClassId(ALLOC_CLASS_VALUE_WRAPPER, alloc_daqdb.class_id);
    DAQ_DEBUG("ARTree alloc class (value wrapper) (" +
              std::to_string(alloc_daqdb.class_id) + ") defined: unit_size=" +
              std::to_string(alloc_daqdb.unit_size) + " units_per_block=" +
              std::to_string(alloc_daqdb.units_per_block));

    // node256
    alloc_daqdb.header_type = POBJ_HEADER_NONE;
    alloc_daqdb.unit_size = sizeof(struct Node256);
    rc = pmemobj_ctl_set(_pm_pool.get_handle(), "heap.alloc_class.new.desc",
                         &alloc_daqdb);
    if (rc)
        throw OperationFailedException(Status(ALLOCATION_ERROR));
    setClassId(ALLOC_CLASS_NODE256, alloc_daqdb.class_id);
    DAQ_DEBUG("ARTree alloc class (node256) (" +
              std::to_string(alloc_daqdb.class_id) + ") defined: unit_size=" +
              std::to_string(alloc_daqdb.unit_size) + " units_per_block=" +
              std::to_string(alloc_daqdb.units_per_block));

    // nodeLeafCompressed
    alloc_daqdb.header_type = POBJ_HEADER_NONE;
    alloc_daqdb.unit_size = sizeof(struct NodeLeafCompressed);
    rc = pmemobj_ctl_set(_pm_pool.get_handle(), "heap.alloc_class.new.desc",
                         &alloc_daqdb);
    if (rc)
        throw OperationFailedException(Status(ALLOCATION_ERROR));
    setClassId(ALLOC_CLASS_NODE_LEAF_COMPRESSED, alloc_daqdb.class_id);
    DAQ_DEBUG("ARTree alloc class (node256) (" +
              std::to_string(alloc_daqdb.class_id) + ") defined: unit_size=" +
              std::to_string(alloc_daqdb.unit_size) + " units_per_block=" +
              std::to_string(alloc_daqdb.units_per_block));
}

void TreeImpl::setClassId(enum ALLOC_CLASS c, unsigned id) {
    _allocClasses[c] = id;
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
        int depth = 0;
        int countNodes = 0;
        int levelsToAllocate = PREALLOC_LEVELS;
        treeRoot = _pm_pool.get_root().get();
        struct pobj_action _actionsArray[ACTION_NUMBER_NODE256];
        int _actionsCounter = 0;
        treeRoot->rootNode = pmemobj_xreserve(
            _pm_pool.get_handle(), &(_actionsArray[_actionsCounter++]),
            sizeof(Node256), VALUE,
            POBJ_CLASS_ID(getClassId(ALLOC_CLASS_NODE256)));
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
            DAQ_DEBUG("Error on publish = " + std::to_string(status));
        }
        _actionsCounter = 0;
        DAQ_DEBUG("root created");
    } else {
        _pm_pool = pool<ARTreeRoot>::open(path, LAYOUT);
        treeRoot = _pm_pool.get_root().get();
        if (treeRoot)
            DAQ_DEBUG("Artree loaded");
        else
            std::cout << "Error on load" << std::endl;
    }

    _initAllocClasses(allocUnitSize);
}

void ARTree::Get(const char *key, int32_t keybytes, void **value, size_t *size,
                 uint8_t *location) {
    ValueWrapper *val =
        tree->findValueInNode(tree->treeRoot->rootNode, key, false);
    if (!val)
        throw OperationFailedException(Status(KEY_NOT_FOUND));
    if (val->location == PMEM && val->locationVolatile.get().value != EMPTY) {
        *value = val->locationPtr.value.get();
        *location = val->location;
    } else if (val->location == DISK) {
        *value = val->locationPtr.IOVptr.get();
        *location = val->location;
    } else {
        throw OperationFailedException(Status(KEY_NOT_FOUND));
    }
    *size = val->size;
}

void ARTree::Get(const char *key, void **value, size_t *size,
                 uint8_t *location) {
    ValueWrapper *val =
        tree->findValueInNode(tree->treeRoot->rootNode, key, false);
    if (!val)
        throw OperationFailedException(Status(KEY_NOT_FOUND));
    if (val->location == PMEM && val->locationVolatile.get().value != EMPTY) {
        *value = val->locationPtr.value.get();
        *location = val->location;
    } else if (val->location == DISK) {
        *value = val->locationPtr.IOVptr.get();
        *location = val->location;
    } else {
        throw OperationFailedException(Status(KEY_NOT_FOUND));
    }
    *size = val->size;
}

void ARTree::Put(const char *key, // copy value from std::string
                 char *value) {
    // printKey(key);
    ValueWrapper *val =
        tree->findValueInNode(tree->treeRoot->rootNode, key, false);
    if (val == nullptr) {
        throw OperationFailedException(Status(KEY_NOT_FOUND));
    }
    if (!tree->treeRoot->initialized) {
        tree->treeRoot->initialized = true;
    }
    val->locationVolatile.get().value = PMEM;
    val->location = PMEM;
    // std::cout<<"PUT OK"<< std::endl;
}

void ARTree::Put(const char *key, int32_t keyBytes, const char *value,
                 int32_t valuebytes) {
    ValueWrapper *val =
        tree->findValueInNode(tree->treeRoot->rootNode, key, false);
    val->locationVolatile.get().value = PMEM;
    val->location = PMEM;
}

void ARTree::Remove(const char *key) {

    ValueWrapper *val =
        tree->findValueInNode(tree->treeRoot->rootNode, key, false);
    if (!val) {
        throw OperationFailedException(Status(KEY_NOT_FOUND));
    }
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

    if (node->type == TYPE256) {
        for (i = 0; i < NODE_SIZE[node->type]; i++) {
            node256 = node;
            if (LEVEL_TYPE[depth] == TYPE256) {
                node256_new = pmemobj_xreserve(
                    _pm_pool.get_handle(), &(actionsArray[actionsCounter]),
                    sizeof(Node256), VALUE,
                    POBJ_CLASS_ID(getClassId(ALLOC_CLASS_NODE256)));
                if (OID_IS_NULL(*(node256_new).raw_ptr())) {
                    DAQ_DEBUG("reserve failed actionsCounter=" +
                              std::to_string(actionsCounter));
                    alloc_err = true;
                    break;
                }
                actionsCounter++;
                node256_new->depth = depth;
                node256_new->type = TYPE256;
                children[i] = node256_new;
            } else if (LEVEL_TYPE[depth] == TYPE_LEAF_COMPRESSED) {
                nodeLeafCompressed_new = pmemobj_xreserve(
                    _pm_pool.get_handle(), &(actionsArray[actionsCounter]),
                    sizeof(NodeLeafCompressed), VALUE,
                    POBJ_CLASS_ID(getClassId(ALLOC_CLASS_NODE256)));
                if (OID_IS_NULL(*(nodeLeafCompressed_new).raw_ptr())) {
                    DAQ_DEBUG("reserve failed actionsCounter=" +
                              std::to_string(actionsCounter));
                    alloc_err = true;
                    break;
                }
                actionsCounter++;
                nodeLeafCompressed_new->depth = depth;
                nodeLeafCompressed_new->type = TYPE_LEAF_COMPRESSED;
                // Temporarily disable the node
                nodeLeafCompressed_new->key = -1;
                children[i] = nodeLeafCompressed_new;
            }

            if (levelsToAllocate > 0) {
                /** @todo handle exceptions */
                allocateFullLevels(node256->children[i], levelsToAllocate,
                                   actionsArray, actionsCounter);
            }
        }
        if (alloc_err) {
            if (actionsCounter)
                pmemobj_cancel(_pm_pool.get_handle(), actionsArray,
                               actionsCounter);
            actionsCounter = 0;
            throw OperationFailedException(Status(ALLOCATION_ERROR));
        }
        for (int i = 0; i < NODE_SIZE[node->type]; i++) {
            node256->children[i] = children[i];
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
        keyCalc = key[KEY_SIZE - current->depth - 1];
        std::bitset<8> x(keyCalc);
        DAQ_DEBUG("findValueInNode: current->depth= " +
                  std::to_string(current->depth) + " keyCalc=" + x.to_string());
        if (current->depth == ((sizeof(LEVEL_TYPE) / sizeof(int) - 1))) {
            // Node Compressed
            nodeLeafCompressed = current;
            if (allocate) {
                static thread_local struct pobj_action
                    actionsArray[ACTION_NUMBER_COMPRESSED];
                int actionsCounter = 0;
                nodeLeafCompressed->child = pmemobj_xreserve(
                    _pm_pool.get_handle(), &(actionsArray[actionsCounter]),
                    sizeof(ValueWrapper), VALUE,
                    POBJ_CLASS_ID(getClassId(ALLOC_CLASS_VALUE_WRAPPER)));
                if (OID_IS_NULL(*(nodeLeafCompressed->child).raw_ptr())) {
                    DAQ_DEBUG("reserve failed actionsCounter=" +
                              std::to_string(actionsCounter));
                    throw OperationFailedException(Status(ALLOCATION_ERROR));
                }
                actionsCounter++;
                int status = pmemobj_publish(_pm_pool.get_handle(),
                                             actionsArray, actionsCounter);
                /** @todo add error check for all pmemobj_publish calls */
                actionsCounter = 0;
                val = reinterpret_cast<ValueWrapper *>(
                    (nodeLeafCompressed->child).raw_ptr());
                /** @todo initialization below temporarily fixes race conditions
                 *        when PUT and GET requests are executed in parallel on
                 *        the same key. This is still not thread-safe.
                */
                val->location = EMPTY;
                // Enable the node
                nodeLeafCompressed->key = keyCalc;
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
        ValueWrapper *val =
            tree->findValueInNode(tree->treeRoot->rootNode, key, true);
        /** @todo handle re-alloc */
        val->location = EMPTY;
        val->actionValue = new pobj_action[1];
        pmemoid poid = pmemobj_xreserve(
            tree->_pm_pool.get_handle(), &(val->actionValue[0]), size, VALUE,
            POBJ_CLASS_ID(tree->getClassId(ALLOC_CLASS_VALUE)));
        if (OID_IS_NULL(poid)) {
            delete val->actionValue;
            val->actionValue = nullptr;
            throw OperationFailedException(Status(ALLOCATION_ERROR));
        }
        val->locationPtr.value = reinterpret_cast<char *>(pmemobj_direct(poid));
        val->size = size;
        *value = reinterpret_cast<char *>(
            pmemobj_direct(*(val->locationPtr.value.raw_ptr())));
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
    ValueWrapper *val =
        tree->findValueInNode(tree->treeRoot->rootNode, key, false);
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
}

} // namespace DaqDB
