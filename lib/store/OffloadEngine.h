
#pragma once

#include <string>

using std::string;
using std::to_string;

namespace FogKV
{

class OffloadEngine {
private:
	unsigned int cluster_size = 0;

public:
    static RTreeEngine* Open(size_t size); // size of cluster
    static void Close(RTreeEngine* kv);

    KVStatus Get(unsigned int cluster);
    template< class Function>
    KVStatus Store(string& value, unsigned int cluster, Function&& f);
    KVStatus Remove(unsigned int cluster);
};
}
