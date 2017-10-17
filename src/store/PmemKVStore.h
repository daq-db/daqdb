/*
 * PmemKVStore.h
 *
 *  Created on: Oct 17, 2017
 *      Author: jradtke
 */

#ifndef SRC_STORE_PMEMKVSTORE_H_
#define SRC_STORE_PMEMKVSTORE_H_

#include <boost/filesystem.hpp>

#include <pmemkv.h>

#include "KVStore.h"

namespace DragonStore
{

class PmemKVStore : public KVStore {
public:
	PmemKVStore(int nodeId);
	virtual ~PmemKVStore();

	/*!
		@copydoc KVStore::Get()
	*/
	virtual KVStatus Get(int32_t limit, int32_t keybytes,
			     int32_t *valuebytes, const char *key,
			     char *value);

	/*!
	 * @copydoc KVStore::Get()
	 */
	virtual KVStatus Get(const string &key, string *valuestr);

	/*!
	 * @copydoc KVStore::Put()
	 */
	virtual KVStatus Put(const string &key, const string &valuestr);

	/*!
	 * @copydoc KVStore::Remove()
	 */
	virtual KVStatus Remove(const string &key);

private:
	boost::filesystem::path kvStoreFile;
	std::unique_ptr<pmemkv::KVEngine> _spStore;
};

} /* namespace DragonStore */

#endif /* SRC_STORE_PMEMKVSTORE_H_ */
