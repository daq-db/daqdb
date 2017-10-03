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

#ifndef SRC_NODE_KVINTERFACE_H_
#define SRC_NODE_KVINTERFACE_H_

#include <pmemkv.h>

namespace DragonNode
{

class KvInterface {
public:
	KvInterface();
	virtual ~KvInterface();

	/**
		 * Copy value for key to buffer
		 *
		 * @param key item identifier
		 * @param limit maximum bytes to copy to buffer
		 * @param value value buffer as C-style string
		 * @param valuebytes buffer bytes actually copied
		 * @return KVStatus
		 */
	KVStatus Get(const string &key, const size_t limit, char *value,
		     uint32_t *valuebytes);

	/**
	 * Append value for key to std::string
	 *
	 * @param key item identifier
	 * @param valuestr item value will be appended to std::string
	 * @return KVStatus
	 */
	KVStatus Get(const string &key, string *valuestr);

	/**
	 * Copy value for key from std::string
	 *
	 * @param key item identifier
	 * @param valuestr value to copy in
	 * @return KVStatus
	 */
	KVStatus Put(const string &key, const string &valuestr);

	/**
	 * Remove value for key
	 *
	 * @param key tem identifier
	 * @return KVStatus
	 */
	KVStatus Remove(const string &key);
};

} /* namespace DragonNode */

#endif /* SRC_NODE_KVINTERFACE_H_ */
