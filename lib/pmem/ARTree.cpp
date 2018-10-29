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
            make_persistent_atomic<Node4>(_pm_pool, treeRoot->rootNode, depth,
                                          LEVEL_TYPE[depth]);
            treeRoot->rootNode->counter = 0;
            allocateFullLevels(treeRoot->rootNode, depth, &countSlots,
                               levelsToAllocate);
            // debug only
            countSlots =
                countSlots * LEVEL_TYPE[(sizeof(LEVEL_TYPE) / sizeof(int)) - 1];
            DAQ_DEBUG("root created, count=" + std::to_string(countSlots));
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

StatusCode ARTree::Put(const char *key, int32_t keyBytes, const char *value,
                       int32_t valuebytes) {
    return StatusCode::Ok;
}

StatusCode ARTree::Remove(const char *key) { return StatusCode::Ok; }

/*
 * Preallocates tree levels
 *
 * @param node pointer to first node
 * @param depth current depth in tree
 * @param count debug counter
 * @param levelsToAllocate how many levels below should be allocated
 *
*/
void TreeImpl::allocateFullLevels(persistent_ptr<Node> node, int depth,
                                  int *count, int levelsToAllocate) {
    depth++;
    levelsToAllocate--;
    persistent_ptr<Node4> node4;
    persistent_ptr<Node256> node256;
    if (levelsToAllocate > 0) {
        if (node->type == 4) {
            for (int i = 0; i < node->type; i++) {
                node4 = node;
                make_persistent_atomic<Node>(_pm_pool, node4->children[i],
                                             depth, LEVEL_TYPE[depth]);
                allocateFullLevels(node4->children[i], depth, count,
                                   levelsToAllocate);
            }
        } else {
            for (int i = 0; i < node->type; i++) {
                node256 = node;
                make_persistent_atomic<Node>(_pm_pool, node256->children[i],
                                             depth, LEVEL_TYPE[depth]);
                allocateFullLevels(node256->children[i], depth, count,
                                   levelsToAllocate);
            }
        }
    }
    if (levelsToAllocate == 0) {
        (*count)++;
    }
}

/*
 * Find value in Tree for a given key, allocate subtree if needed.
 *
 * @param current marks beggining of search
 * @param key pointer to searched key
 * @param allocate flag to specify if subtree should be allocated if key not found
 * @return pointer to value
 */
ValueWrapper *TreeImpl::findValueInNode(persistent_ptr<Node> current,
                                    const char *key, bool allocate) {
    /*thread_local ValueWrapper *cachedVal = nullptr;
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
    };*/
    int keyCalc;
    persistent_ptr<Node4> node4;
    persistent_ptr<Node256> node256;
    while (1) {
        keyCalc = key[current->depth];
        std::bitset<8> x(keyCalc);
        DAQ_DEBUG(x.to_string());
        if (current->depth == ((sizeof(LEVEL_TYPE) / sizeof(int) - 1))) {
            DAQ_DEBUG("end");
            break;
        }
        if (current->type == 4) {
            node4 = current;
            int i=0;
            for(i=0;i<4;i++) {
                DAQ_DEBUG("checking node4, i = " + std::to_string(i) );
                if(node4->keys[i] == keyCalc) {
                    //Node4 should count number of elements but this requires atomic add
                    //To be sure that correct key was found we need to check if value ptr exists
                    //It is only observed when keyCalc == 0 and keys array is not initialized
                    if(node4->children[i])
                    {
                        DAQ_DEBUG("found correct key");
                        current = node4->children[i];
                        break;
                    }
                    else{
                        DAQ_DEBUG("Found empty key");
                        i=4;
                        break;
                    }
                }
            }
            if(i==4)
            {
                DAQ_DEBUG("not found");
                break;
            }

        }
        else{
            node256 = current;
        }
        //current = current->children[keyCalc];
    }


    return nullptr;
}


StatusCode ARTree::AllocValueForKey(const char *key, size_t size,
                                    char **value) {
    int depth = 0;
    DAQ_DEBUG("AllocValueForKey" );
    if (tree->treeRoot->rootNode) {
        DAQ_DEBUG("root exists");
        ValueWrapper *val = tree->findValueInNode(tree->treeRoot->rootNode, key, true);
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
