#include "RTreeEngine.h"
#include "RTree.h"
namespace FogKV
{
RTreeEngine* RTreeEngine::Open(const string& engine,            // open storage engine
                         const string& path,              // path to persistent pool
                         size_t size)
	{
		return new FogKV::RTree(path, size);
	}

   void RTreeEngine::Close(RTreeEngine* kv){}                       // close storage engine

}
