#include "RTree.h"
#include <iostream>

namespace FogKV
{
	#define LAYOUT "rtree"

	RTree::RTree(const string& _path, const size_t size) {
		std::cout << "Creating RTree " << std::endl;
		tree = new Tree(_path, size);
	}

	RTree::~RTree() {
		// TODO Auto-generated destructor stub
	}
	Tree::Tree(const string& _path, const size_t size) {
		const char *path = "/mnt/pmem/test.pm";

		if (!boost::filesystem::exists(path)) {
				std::cout << "Creating pool " << std::endl;
				try {
					_pm_pool = pool<TreeRoot>::create(
					path, LAYOUT, 800*PMEMOBJ_MIN_POOL, S_IWUSR | S_IRUSR);
				} catch (pmem::pool_error &pe) {std::cout << "Error on create" << pe.what();}
				try {
					transaction::exec_tx(_pm_pool, [&] {
						treeRoot = _pm_pool.get_root().get();
						treeRoot->rootNode = make_persistent<Node>(false, 0);
						treeRoot->level_bits = log2(LEVEL_SIZE);
						treeRoot->tree_heigh = KEY_SIZE/treeRoot->level_bits;
						int count = 0;
						allocateLevel(treeRoot->rootNode, treeRoot->rootNode->depth, &count);
					});
				} catch (std::exception &e) {
					std::cout << "Error " << e.what();
				}

			}
			else
			{
				std::cout << "Opening existing pool " << std::endl;
				_pm_pool = pool<TreeRoot>::open(path, LAYOUT);
				treeRoot = _pm_pool.get_root().get();
			}


		std::cout << "Tree constructor " << std::endl;

	}
	KVStatus RTree::Get(int32_t limit,                    // copy value to fixed-size buffer
							int32_t keybytes,
							int32_t* valuebytes,
							const char* key,
							char* value) {
	   return OK;
	}
	KVStatus RTree::Get(const string& key,                // append value to std::string
						string* value) {
		ValueWrapper * val = tree->findValueInNode(tree->treeRoot->rootNode, atoi(key.c_str()));
		value->append(val->value.get());

		return OK;
	}
	KVStatus RTree::Put(const string& key,                // copy value from std::string
						const string& value) {
		ValueWrapper * val = tree->findValueInNode(tree->treeRoot->rootNode, atoi(key.c_str()));
		try {
			transaction::exec_tx(tree->_pm_pool, [&] {
				val->value = pmemobj_tx_alloc(value.length(),1);
			});
		} catch (std::exception &e) {
			std::cout << "Error " << e.what();
		}
		memcpy(val->value.get(), value.data(), value.length());
		return OK;
	}
	KVStatus RTree::Remove(const string& key) {
	   return OK;
	}

	void Tree::allocateLevel(persistent_ptr<Node> current, int depth, int * count) {
		int i;
		depth++;

		for (i=0; i<LEVEL_SIZE; i++)
		{
			bool isLast = false;
			if(depth==((treeRoot->tree_heigh)-1))
			{
				isLast = true;
			}
			if(depth==treeRoot->tree_heigh)
			{
				(*count)++;	//debug only
			}

			current->children[i] = make_persistent<Node>(isLast,depth);

			if(depth<treeRoot->tree_heigh)
			{
				allocateLevel(current->children[i], depth, count);
			}

		}
	}

	ValueWrapper * Tree::findValueInNode(persistent_ptr<Node> current, int key) {
		int key_shift = (2*(treeRoot->tree_heigh - current->depth-1));
		int key_calc = key >> key_shift;
		int mask = ~(UINT_MAX << treeRoot->level_bits);
		key_calc = key_calc & mask;

		if(current->depth!=(treeRoot->tree_heigh-1))
		{
			return findValueInNode(current->children[key_calc], key);
		}
		return &(current->valueNode[key_calc]);
	}
}
