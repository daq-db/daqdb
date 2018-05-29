#ifndef LIB_STORE_RTREE_H_
#define LIB_STORE_RTREE_H_

#include "RTreeEngine.h"

#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/make_persistent_array.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

#include <boost/filesystem.hpp>
#include <boost/filesystem/operations.hpp>

#include <cmath>
#include <climits>

using namespace pmem::obj;

//Number of keys slots inside Node (not key bits)
#define LEVEL_SIZE 4
// Number of key bits
#define KEY_SIZE 16

namespace FogKV
{

	struct ValueWrapper {
		p<int> location;
		persistent_ptr<char> value;
		string getString();
	};

	struct Node {
		explicit Node (bool _isEnd, int _depth) : isEnd(_isEnd), depth(_depth)
		{
			//children = make_persistent<Node[LEVEL_SIZE]>();
			if(isEnd)
			{
				valueNode = make_persistent<ValueWrapper[LEVEL_SIZE]>();
			}
		}
		persistent_ptr<Node> children[LEVEL_SIZE];
		bool isEnd;
		int depth;
		//persistent_ptr<int[LEVEL_SIZE]> values;		//only for leaves
		persistent_ptr<ValueWrapper[LEVEL_SIZE]> valueNode;		//only for leaves
	};


	struct TreeRoot {
		persistent_ptr<Node> rootNode;
		p<int> level_bits;
		p<int> tree_heigh;
	};

	class Tree {
		public:
			Tree(const string& path, const size_t size);
			ValueWrapper * findValueInNode(persistent_ptr<Node> current, int key);
			void allocateLevel(persistent_ptr<Node> current, int depth, int * count);
			//persistent_ptr<Node> treeRoot;
			TreeRoot* treeRoot;
			pool<TreeRoot> _pm_pool;
		private:


		};

	class RTree: public FogKV::RTreeEngine {
	public:
		RTree(const string& path, const size_t size);
		~RTree();
		string Engine() final { return "RTree"; }
		KVStatus Get(int32_t limit,                            // copy value to fixed-size buffer
		                 int32_t keybytes,
		                 int32_t* valuebytes,
		                 const char* key,
		                 char* value) final;
		KVStatus Get(const string& key,                        // append value to std::string
					 string* value) final;
		KVStatus Put(const string& key,                        // copy value from std::string
					 const string& value) final;
		KVStatus Remove(const string& key) final;              // remove value for key
	private:
		Tree * tree;
	};


}
#endif /* LIB_STORE_RTREE_H_ */
