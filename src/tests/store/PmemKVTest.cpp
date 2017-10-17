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

#include <iostream>
#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>

#include <libpmemobj/pool_base.h>
#include <pmemkv.h>

using namespace std;
using namespace pmemkv;

namespace ut = boost::unit_test;

namespace
{
const unsigned short dhtBackBonePort = 11000;
const string pmemKvEngine = "kvtree";
const string pmemKvPath = "/dev/shm/forkv_test_pmemkv";
};

BOOST_AUTO_TEST_SUITE(KVStoreTests)

BOOST_AUTO_TEST_CASE(KVStoreTests_PutGetRemove, *ut::description(""))
{
	std::unique_ptr<KVEngine> spPmemKV(KVEngine::Open(
		pmemKvEngine, pmemKvPath, PMEMOBJ_MIN_POOL));

	auto actionStatus = spPmemKV->Put("key1", "value1");

	BOOST_TEST(actionStatus == OK, "Put item to pmemKV store");

	string resultValue;
	actionStatus = spPmemKV->Get("key1", &resultValue);

	BOOST_TEST(actionStatus == KVStatus::OK, "Get item from pmemKV store");
	BOOST_TEST(resultValue == "value1");

	actionStatus = spPmemKV->Remove("key1");
	BOOST_TEST(actionStatus == KVStatus::OK, "Remove item from pmemKV store");

	actionStatus = spPmemKV->Get("key1", &resultValue);
	BOOST_TEST(actionStatus == KVStatus::NOT_FOUND, "Check if item removed from pmemKV store");

	boost::filesystem::remove(pmemKvPath);
}

BOOST_AUTO_TEST_SUITE_END()
