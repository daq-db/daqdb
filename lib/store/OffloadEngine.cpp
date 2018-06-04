#include "OffloadEngine.h"
namespace FogKV
{
	OffloadEngine* OffloadEngine::Open(size_t size)
	{
		/** TODO: open device here */
		cluster_size = size; /* in blocks */
	}

		void OffloadEngine::Close(RTreeEngine* kv)
	{

	}
		KVStatus Get(unsigned int cluster)
	{
		rc = spdk_bdev_read_blocks(desc, /* bdev descriptor */
				 ch, /* current channel for thread */
				value, /* buffer for data */
				cluster * cluster_size; /* cluster number */
				cluster_size,  /* number of pages per cluster */
				cb, /* completion callback */
				cb_arg); /* context */
	}

	template< class Function>
	KVStatus Store(string& value, unsigned int cluster,  Function&& f)
	{
		rc = spdk_bdev_write_blocks(desc, /* bdev descriptor */
				 ch, /* current channel for thread */
				value, /* buffer with data */
				cluster * cluster_size; /* cluster number */
				cluster_size,  /* number of pages per cluster */
				cb, /* completion callback */
				cb_arg); /* context */
	}
		KVStatus Remove(unsigned int cluster)
	{

	}
}
