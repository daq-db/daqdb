/**
 * Copyright 2018, Intel Corporation
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

#include <cstdint>

#include "../../lib/store/OffloadFreeList.cpp"
#include "../../lib/store/RTree.h"

#define BOOST_TEST_MAIN
#include <boost/filesystem.hpp>
#include <boost/test/unit_test.hpp>

#include <fakeit.hpp>

namespace ut = boost::unit_test;

using namespace fakeit;

#define BOOST_TEST_DETECT_MEMORY_LEAK 1

#define TEST_POOL_FREELIST_FILENAME "test_file.pm"
#define TEST_POOL_FREELIST_SIZE 10ULL * 1024 * 1024
#define LAYOUT "queue"
#define CREATE_MODE_RW (S_IWUSR | S_IRUSR)

#define FREELIST_MAX_LBA 512

pool<FogKV::OffloadFreeList> *g_poolFreeList = nullptr;

bool txAbortPred(const pmem::manual_tx_abort &ex) { return true; }

struct OffloadFreeListGlobalFixture {
    OffloadFreeListGlobalFixture() {
        if (boost::filesystem::exists(TEST_POOL_FREELIST_FILENAME))
            boost::filesystem::remove(TEST_POOL_FREELIST_FILENAME);
        poolFreeList = pool<FogKV::OffloadFreeList>::create(
            TEST_POOL_FREELIST_FILENAME, LAYOUT, TEST_POOL_FREELIST_SIZE,
            CREATE_MODE_RW);
        g_poolFreeList = &poolFreeList;
    }
    ~OffloadFreeListGlobalFixture() {
        if (boost::filesystem::exists(TEST_POOL_FREELIST_FILENAME))
            boost::filesystem::remove(TEST_POOL_FREELIST_FILENAME);
    }
    pool<FogKV::OffloadFreeList> poolFreeList;
};

BOOST_GLOBAL_FIXTURE(OffloadFreeListGlobalFixture);

struct OffloadFreeListTestFixture {
    OffloadFreeListTestFixture() {
        freeLbaList = g_poolFreeList->get_root().get();
    }
    ~OffloadFreeListTestFixture() {
        if (freeLbaList != nullptr)
            freeLbaList->Clear(*g_poolFreeList);
        freeLbaList->maxLba = 0;
    }
    FogKV::OffloadFreeList *freeLbaList = nullptr;
};

BOOST_FIXTURE_TEST_CASE(GetLbaInit, OffloadFreeListTestFixture) {

    freeLbaList->Push(*g_poolFreeList, -1);
    freeLbaList->maxLba = FREELIST_MAX_LBA;
    auto firstLBA = freeLbaList->GetFreeLba(*g_poolFreeList);
    BOOST_CHECK_EQUAL(firstLBA, 1);
}

BOOST_FIXTURE_TEST_CASE(GetLbaInitPhase, OffloadFreeListTestFixture) {

    freeLbaList->Push(*g_poolFreeList, -1);
    freeLbaList->maxLba = FREELIST_MAX_LBA;

    const int lbaCount = 100;

    uint8_t index;
    long lba = 0;
    for (index = 0; index < lbaCount; index++)
        lba = freeLbaList->GetFreeLba(*g_poolFreeList);

    BOOST_CHECK_EQUAL(lba, lbaCount);
}

BOOST_FIXTURE_TEST_CASE(GetLbaAfterInit_MaxLba_NotSet,
                        OffloadFreeListTestFixture) {
    freeLbaList->Push(*g_poolFreeList, -1);

    BOOST_CHECK_EXCEPTION(freeLbaList->GetFreeLba(*g_poolFreeList),
                          pmem::manual_tx_abort, txAbortPred);
}

BOOST_FIXTURE_TEST_CASE(GetLastInitLba, OffloadFreeListTestFixture) {
    freeLbaList->Push(*g_poolFreeList, -1);
    freeLbaList->maxLba = FREELIST_MAX_LBA;

    long lba = 0;
    unsigned int index;
    for (index = 0; index < FREELIST_MAX_LBA - 1; index++)
        lba = freeLbaList->GetFreeLba(*g_poolFreeList);

    BOOST_CHECK_EQUAL(lba, index);
}

BOOST_FIXTURE_TEST_CASE(GetLbaAfterInit_FirstLba, OffloadFreeListTestFixture) {

    freeLbaList->Push(*g_poolFreeList, -1);
    freeLbaList->maxLba = FREELIST_MAX_LBA;

    long lba = 0;
    unsigned int index;
    for (index = 0; index < FREELIST_MAX_LBA; index++)
        lba = freeLbaList->GetFreeLba(*g_poolFreeList);

    BOOST_CHECK_EQUAL(lba, 0);
}

BOOST_FIXTURE_TEST_CASE(GetLbaAfterInit_CheckRemoved,
                        OffloadFreeListTestFixture) {
    freeLbaList->Push(*g_poolFreeList, -1);
    freeLbaList->maxLba = FREELIST_MAX_LBA;
    const long freeElementLba = 100;

    long lba = 0;
    unsigned int index;
    for (index = 0; index < FREELIST_MAX_LBA; index++)
        lba = freeLbaList->GetFreeLba(*g_poolFreeList);

    freeLbaList->Push(*g_poolFreeList, freeElementLba);
    lba = freeLbaList->GetFreeLba(*g_poolFreeList);
    BOOST_CHECK_EQUAL(lba, freeElementLba);
    freeLbaList->Push(*g_poolFreeList, freeElementLba + 1);
    lba = freeLbaList->GetFreeLba(*g_poolFreeList);
    BOOST_CHECK_EQUAL(lba, freeElementLba + 1);

    freeLbaList->Push(*g_poolFreeList, freeElementLba);
    freeLbaList->Push(*g_poolFreeList, freeElementLba + 1);
    lba = freeLbaList->GetFreeLba(*g_poolFreeList);
    BOOST_CHECK_EQUAL(lba, freeElementLba);
    lba = freeLbaList->GetFreeLba(*g_poolFreeList);
    BOOST_CHECK_EQUAL(lba, freeElementLba + 1);
}

BOOST_FIXTURE_TEST_CASE(GetLbaAfterInit_FullDisk, OffloadFreeListTestFixture) {
    freeLbaList->Push(*g_poolFreeList, -1);
    freeLbaList->maxLba = FREELIST_MAX_LBA;

    long lba = 0;
    unsigned int index;
    for (index = 0; index < FREELIST_MAX_LBA; index++)
        lba = freeLbaList->GetFreeLba(*g_poolFreeList);

    BOOST_CHECK_EXCEPTION(freeLbaList->GetFreeLba(*g_poolFreeList),
                          pmem::manual_tx_abort, txAbortPred);
}
