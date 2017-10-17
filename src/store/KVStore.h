/*
 * KVStore.h
 *
 *  Created on: Oct 17, 2017
 *      Author: jradtke
 */

#ifndef SRC_STORE_KVSTORE_H_
#define SRC_STORE_KVSTORE_H_

#include "KVInterface.h"

namespace DragonStore
{

class KVStore : public KVInterface {
public:
	KVStore();
	virtual ~KVStore();

	/*!
	 * Copy value for key to buffer
	 *
	 * @param limit maximum bytes to copy to buffer
	 * @param keybytes key buffer bytes actually copied
	 * @param valuebytes value buffer bytes actually copied
	 * @param key item identifier
	 * @param value value buffer as C-style string
	 * @return
	 */
	virtual KVStatus Get(int32_t limit, int32_t keybytes,
			     int32_t *valuebytes, const char *key,
			     char *value);

	/**
	 * Append value for key to std::string
	 *
	 * @param key item identifier
	 * @param valuestr item value will be appended to std::string
	 * @return KVStatus
	 */
	virtual KVStatus Get(const string &key, string *valuestr);

	/**
	 * Copy value for key from std::string
	 *
	 * @param key item identifier
	 * @param valuestr value to copy in
	 * @return KVStatus
	 */
	virtual KVStatus Put(const string &key, const string &valuestr);

	/**
	 * Remove value for key
	 *
	 * @param key tem identifier
	 * @return KVStatus
	 */
	virtual KVStatus Remove(const string &key);
};

} /* namespace DragonStore */

#endif /* SRC_STORE_KVSTORE_H_ */
