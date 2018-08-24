/**
 * Copyright 2018 Intel Corporation.
 *
 * This software and the related documents are Intel copyrighted materials,
 * and your use of them is governed by the express license under which they
 * were provided to you (Intel OBL Internal Use License).
 * Unless the License provides otherwise, you may not use, modify, copy,
 * publish, distribute, disclose or transmit this software or the related
 * documents without Intel's prior written permission.
 *
 * This software and the related documents are provided as is, with no
 * express or implied warranties, other than those that are expressly
 * stated in the License.
 */

#include <cstdint>

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
    char valRef[] = "abc";
    size_t sizeRef = 3;

    When(OverloadedMethod(rtreeMock, Get,
                          FogKV::StatusCode(const char *, int32_t, void **,
                                            size_t *, uint8_t *))
             .Using(expectedKey, expectedKeySize, _, _, _))
        .AlwaysDo([&](const char *key, int32_t keySize, void **val,
                      size_t *valSize, uint8_t *) {
            *val = valRef;
            *valSize = sizeRef;
            return FogKV::StatusCode::Ok;
        });

    FogKV::RqstPooler &pooler = poolerMock.get();
    FogKV::RTreeEngine &rtree = rtreeMock.get();
    pooler.rtree.reset(&rtree);

    pooler.processArray[0] =
        new FogKV::RqstMsg(FogKV::RqstOperation::GET, expectedKey,
                           expectedKeySize, nullptr, 0, nullptr);
    pooler.processArrayCount = 1;

    pooler.ProcessMsg();

    Verify(OverloadedMethod(rtreeMock, Get,
                            FogKV::StatusCode(const char *, int32_t, void **,
                                              size_t *, uint8_t *)))
        .Exactly(1);
}

BOOST_AUTO_TEST_CASE(ProcessMultipleGetRqst) {

    Mock<FogKV::RqstPooler> poolerMock;
    Mock<FogKV::RTree> rtreeMock;
    char valRef[] = "abc";
    size_t sizeRef = 3;

    When(OverloadedMethod(rtreeMock, Get,
                          FogKV::StatusCode(const char *, int32_t, void **,
                                            size_t *, uint8_t *))
             .Using(expectedKey, expectedKeySize, _, _, _))
        .AlwaysDo([&](const char *key, int32_t keySize, void **val,
                      size_t *valSize, uint8_t *) {
            *val = valRef;
            *valSize = sizeRef;
            return FogKV::StatusCode::Ok;
        });

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
                            FogKV::StatusCode(const char *, int32_t, void **,
                                              size_t *, uint8_t *)))
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
    char valRef[] = "abc";
    size_t sizeRef = 3;

    When(OverloadedMethod(rtreeMock, Get,
                          FogKV::StatusCode(const char *, int32_t, void **,
                                            size_t *, uint8_t *))
             .Using(expectedKey, expectedKeySize, _, _, _))
        .Do([&](const char *key, int32_t keySize, void **val, size_t *valSize,
                uint8_t *) {
            *val = valRef;
            *valSize = sizeRef;
            return FogKV::StatusCode::Ok;
        })
        .Do([&](const char *key, int32_t keySize, void **val, size_t *valSize,
                uint8_t *) {
            *val = valRef;
            *valSize = sizeRef;
            return FogKV::StatusCode::KeyNotFound;
        });

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
                            FogKV::StatusCode(const char *, int32_t, void **,
                                              size_t *, uint8_t *)))
        .Exactly(2);
}
