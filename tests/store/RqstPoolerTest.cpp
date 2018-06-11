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

#include "../../lib/store/RTree.h"
#include "../../lib/store/RqstPooler.cpp"

#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>

#include <fakeit.hpp>

namespace ut = boost::unit_test;

using namespace fakeit;

static const char *expectedKey = "key123";
static const size_t expectedKeySize = 6;
static const char *expectedVal = "val123";
static const size_t expectedValSize = 6;

#define BOOST_TEST_DETECT_MEMORY_LEAK 1

BOOST_AUTO_TEST_CASE(ProcessEmptyRing) {
    Mock<FogKV::RqstPooler> poolerMock;
    Mock<FogKV::RTree> rtreeMock;

    When(OverloadedMethod(
             rtreeMock, Put,
             FogKV::StatusCode(const char *, int32_t, const char *, int32_t)))
        .Return(FogKV::StatusCode::Ok);

    FogKV::RqstPooler &pooler = poolerMock.get();
    FogKV::RTreeEngine &rtree = rtreeMock.get();
    pooler.rtree.reset(&rtree);

    pooler.ProcessMsg();
    VerifyNoOtherInvocations(OverloadedMethod(
        rtreeMock, Put,
        FogKV::StatusCode(const char *, int32_t, const char *, int32_t)));
}

BOOST_AUTO_TEST_CASE(ProcessPutRqst) {

    Mock<FogKV::RqstPooler> poolerMock;
    Mock<FogKV::RTree> rtreeMock;

    When(OverloadedMethod(
             rtreeMock, Put,
             FogKV::StatusCode(const char *, int32_t, const char *, int32_t))
             .Using(expectedKey, expectedKeySize, expectedVal, expectedValSize))
        .Return(FogKV::StatusCode::Ok);

    FogKV::RqstPooler &pooler = poolerMock.get();
    FogKV::RTreeEngine &rtree = rtreeMock.get();
    pooler.rtree.reset(&rtree);

    pooler.processArray[0] = new FogKV::RqstMsg(
        FogKV::RqstOperation::PUT, expectedKey, expectedKeySize, expectedVal,
        expectedValSize, nullptr);
    pooler.processArrayCount = 1;

    pooler.ProcessMsg();

    Verify(OverloadedMethod(
               rtreeMock, Put,
               FogKV::StatusCode(const char *, int32_t, const char *, int32_t)))
        .Exactly(1);
}

BOOST_AUTO_TEST_CASE(ProcessMultiplePutRqst) {

    Mock<FogKV::RqstPooler> poolerMock;
    Mock<FogKV::RTree> rtreeMock;

    When(OverloadedMethod(
             rtreeMock, Put,
             FogKV::StatusCode(const char *, int32_t, const char *, int32_t))
             .Using(expectedKey, expectedKeySize, expectedVal, expectedValSize))
        .AlwaysReturn(FogKV::StatusCode::Ok);

    FogKV::RqstPooler &pooler = poolerMock.get();
    FogKV::RTreeEngine &rtree = rtreeMock.get();
    pooler.rtree.reset(&rtree);

    for (int index = 0; index < DEQUEUE_RING_LIMIT; index++) {
        pooler.processArray[index] = new FogKV::RqstMsg(
            FogKV::RqstOperation::PUT, expectedKey, expectedKeySize,
            expectedVal, expectedValSize, nullptr);
    }
    pooler.processArrayCount = DEQUEUE_RING_LIMIT;

    pooler.ProcessMsg();

    Verify(OverloadedMethod(
               rtreeMock, Put,
               FogKV::StatusCode(const char *, int32_t, const char *, int32_t)))
        .Exactly(DEQUEUE_RING_LIMIT);
}

BOOST_AUTO_TEST_CASE(ProcessGetRqst) {

    Mock<FogKV::RqstPooler> poolerMock;
    Mock<FogKV::RTree> rtreeMock;

    When(OverloadedMethod(rtreeMock, Get,
                          FogKV::StatusCode(const char *, int32_t, string *))
             .Using(expectedKey, expectedKeySize, _))
        .Return(FogKV::StatusCode::Ok);

    FogKV::RqstPooler &pooler = poolerMock.get();
    FogKV::RTreeEngine &rtree = rtreeMock.get();
    pooler.rtree.reset(&rtree);

    pooler.processArray[0] =
        new FogKV::RqstMsg(FogKV::RqstOperation::GET, expectedKey,
                           expectedKeySize, nullptr, 0, nullptr);
    pooler.processArrayCount = 1;

    pooler.ProcessMsg();

    Verify(OverloadedMethod(rtreeMock, Get,
                            FogKV::StatusCode(const char *, int32_t, string *)))
        .Exactly(1);
}

BOOST_AUTO_TEST_CASE(ProcessMultipleGetRqst) {

    Mock<FogKV::RqstPooler> poolerMock;
    Mock<FogKV::RTree> rtreeMock;

    When(OverloadedMethod(rtreeMock, Get,
                          FogKV::StatusCode(const char *, int32_t, string *))
             .Using(expectedKey, expectedKeySize, _))
        .AlwaysReturn(FogKV::StatusCode::Ok);

    FogKV::RqstPooler &pooler = poolerMock.get();
    FogKV::RTreeEngine &rtree = rtreeMock.get();
    pooler.rtree.reset(&rtree);

    for (int index = 0; index < DEQUEUE_RING_LIMIT; index++) {
        pooler.processArray[index] =
            new FogKV::RqstMsg(FogKV::RqstOperation::GET, expectedKey,
                               expectedKeySize, nullptr, 0, nullptr);
    }
    pooler.processArrayCount = DEQUEUE_RING_LIMIT;

    pooler.ProcessMsg();

    Verify(OverloadedMethod(rtreeMock, Get,
                            FogKV::StatusCode(const char *, int32_t, string *)))
        .Exactly(DEQUEUE_RING_LIMIT);
}

BOOST_AUTO_TEST_CASE(ProcessPutTestCallback) {

    Mock<FogKV::RqstPooler> poolerMock;
    Mock<FogKV::RTree> rtreeMock;

    When(OverloadedMethod(
             rtreeMock, Put,
             FogKV::StatusCode(const char *, int32_t, const char *, int32_t))
             .Using(expectedKey, expectedKeySize, expectedVal, expectedValSize))
        .Return(FogKV::StatusCode::Ok)
        .Return(FogKV::StatusCode::UnknownError);

    FogKV::RqstPooler &pooler = poolerMock.get();
    FogKV::RTreeEngine &rtree = rtreeMock.get();
    pooler.rtree.reset(&rtree);

    pooler.processArray[0] = new FogKV::RqstMsg(
        FogKV::RqstOperation::PUT, expectedKey, expectedKeySize, expectedVal,
        expectedValSize,
        [&](FogKV::KVStoreBase *kvs, FogKV::Status status, const char *key,
            const size_t keySize, const char *value, const size_t valueSize) {
            BOOST_REQUIRE(status.ok());
            BOOST_CHECK_EQUAL(key, expectedKey);
            BOOST_CHECK_EQUAL(keySize, expectedKeySize);
            BOOST_CHECK_EQUAL(value, expectedVal);
            BOOST_CHECK_EQUAL(valueSize, expectedValSize);
        });

    pooler.processArray[1] = new FogKV::RqstMsg(
        FogKV::RqstOperation::PUT, expectedKey, expectedKeySize, expectedVal,
        expectedKeySize,
        [&](FogKV::KVStoreBase *kvs, FogKV::Status status, const char *key,
            const size_t keySize, const char *value, const size_t valueSize) {
            BOOST_REQUIRE(!status.ok());
            BOOST_CHECK_EQUAL(key, expectedKey);
            BOOST_CHECK_EQUAL(keySize, expectedKeySize);
            BOOST_CHECK_EQUAL(value, expectedVal);
            BOOST_CHECK_EQUAL(valueSize, expectedValSize);
        });
    pooler.processArrayCount = 2;

    pooler.ProcessMsg();

    Verify(OverloadedMethod(
               rtreeMock, Put,
               FogKV::StatusCode(const char *, int32_t, const char *, int32_t)))
        .Exactly(2);
}

BOOST_AUTO_TEST_CASE(ProcessGetTestCallback) {

    Mock<FogKV::RqstPooler> poolerMock;
    Mock<FogKV::RTree> rtreeMock;

    When(OverloadedMethod(rtreeMock, Get,
                          FogKV::StatusCode(const char *, int32_t, string *))
             .Using(expectedKey, expectedKeySize, _))
        .Return(FogKV::StatusCode::Ok)
        .Return(FogKV::StatusCode::KeyNotFound);

    FogKV::RqstPooler &pooler = poolerMock.get();
    FogKV::RTreeEngine &rtree = rtreeMock.get();
    pooler.rtree.reset(&rtree);

    pooler.processArray[0] = new FogKV::RqstMsg(
        FogKV::RqstOperation::GET, expectedKey, expectedKeySize, nullptr, 0,
        [&](FogKV::KVStoreBase *kvs, FogKV::Status status, const char *key,
            const size_t keySize, const char *value, const size_t valueSize) {
            BOOST_REQUIRE(status.ok());
            BOOST_CHECK_EQUAL(key, expectedKey);
            BOOST_CHECK_EQUAL(keySize, expectedKeySize);
        });

    pooler.processArray[1] = new FogKV::RqstMsg(
        FogKV::RqstOperation::GET, expectedKey, expectedKeySize, nullptr, 0,
        [&](FogKV::KVStoreBase *kvs, FogKV::Status status, const char *key,
            const size_t keySize, const char *value, const size_t valueSize) {
            BOOST_REQUIRE(!status.ok());
            BOOST_CHECK_EQUAL(key, expectedKey);
            BOOST_CHECK_EQUAL(keySize, expectedKeySize);
        });
    pooler.processArrayCount = 2;

    pooler.ProcessMsg();

    Verify(OverloadedMethod(rtreeMock, Get,
                            FogKV::StatusCode(const char *, int32_t, string *)))
        .Exactly(2);
}
