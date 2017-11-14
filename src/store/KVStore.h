/**
 * Copyright 2017, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of the copyright holder nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef SRC_STORE_KVSTORE_H_
#define SRC_STORE_KVSTORE_H_

#include "KVInterface.h"

namespace Dragon
{

/*!
 * Class that defines KV store interface
 */

class Memory {
	virtual KVStatus Getbuf();
	
}


class KVStore : public KVInterface {
public:
	KVStore();
	virtual ~KVStore();

	/*!
	 * Copy value for key to buffer
	 *
	 * @param key
	 * @param keybytes key buffer bytes actually copied
	 * @param valuebytes value buffer bytes actually copied
	 * @param key item identifier
	 * @param value value buffer as C-style string
	 * @return
	 */
	virtual KVStatus GetLock(int *key, char **value, callback, arg);
	
	virtual KVStatus Put(int *key, char *value, callback, arg);

	virtual KVStatus Commit(int *key, char *value, callback, arg);

	virtual KVStatus Remove(int key);
};

} /* namespace DragonStore */

#endif /* SRC_STORE_KVSTORE_H_ */
