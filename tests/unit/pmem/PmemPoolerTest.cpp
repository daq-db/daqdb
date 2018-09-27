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

#include "../../../lib/pmem/PmemPooler.cpp"
#include "../../lib/pmem/RTree.h"

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
    Mock<DaqDB::PmemPooler> poolerMock;
    Mock<DaqDB::RTree> rtreeMock;

    When(OverloadedMethod(
             rtreeMock, Put,
             DaqDB::StatusCode(const char *, int32_t, const char *, int32_t)))
        .Return(DaqDB::StatusCode::Ok);

    DaqDB::PmemPooler &pooler = poolerMock.get();
    DaqDB::RTreeEngine &rtree = rtreeMock.get();
    pooler.rtree.reset(&rtree);

    pooler.Process();
    VerifyNoOtherInvocations(OverloadedMethod(
        rtreeMock, Put,
        DaqDB::StatusCode(const char *, int32_t, const char *, int32_t)));
}

BOOST_AUTO_TEST_CASE(ProcessPutRqst) {

    Mock<DaqDB::PmemPooler> poolerMock;
    Mock<DaqDB::RTree> rtreeMock;

    When(OverloadedMethod(
             rtreeMock, Put,
             DaqDB::StatusCode(const char *, int32_t, const char *, int32_t))
             .Using(expectedKey, expectedKeySize, expectedVal, expectedValSize))
        .Return(DaqDB::StatusCode::Ok);

    DaqDB::PmemPooler &pooler = poolerMock.get();
    DaqDB::RTreeEngine &rtree = rtreeMock.get();
    pooler.rtree.reset(&rtree);

    pooler.requests = new DaqDB::PmemRqst *[1];
    pooler.requests[0] = new DaqDB::PmemRqst(
        DaqDB::RqstOperation::PUT, expectedKey, expectedKeySize, expectedVal,
        expectedValSize, nullptr);
    pooler.requestCount = 1;

    pooler.Process();

    Verify(OverloadedMethod(
               rtreeMock, Put,
               DaqDB::StatusCode(const char *, int32_t, const char *, int32_t)))
        .Exactly(1);

    delete[] pooler.requests;
}

BOOST_AUTO_TEST_CASE(ProcessMultiplePutRqst) {

    Mock<DaqDB::PmemPooler> poolerMock;
    Mock<DaqDB::RTree> rtreeMock;

    When(OverloadedMethod(
             rtreeMock, Put,
             DaqDB::StatusCode(const char *, int32_t, const char *, int32_t))
             .Using(expectedKey, expectedKeySize, expectedVal, expectedValSize))
        .AlwaysReturn(DaqDB::StatusCode::Ok);

    DaqDB::PmemPooler &pooler = poolerMock.get();
    DaqDB::RTreeEngine &rtree = rtreeMock.get();
    pooler.rtree.reset(&rtree);

    pooler.requests = new DaqDB::PmemRqst *[DEQUEUE_RING_LIMIT];
    for (int index = 0; index < DEQUEUE_RING_LIMIT; index++) {
        pooler.requests[index] = new DaqDB::PmemRqst(
            DaqDB::RqstOperation::PUT, expectedKey, expectedKeySize,
            expectedVal, expectedValSize, nullptr);
    }
    pooler.requestCount = DEQUEUE_RING_LIMIT;

    pooler.Process();

    Verify(OverloadedMethod(
               rtreeMock, Put,
               DaqDB::StatusCode(const char *, int32_t, const char *, int32_t)))
        .Exactly(DEQUEUE_RING_LIMIT);
    delete[] pooler.requests;
}

BOOST_AUTO_TEST_CASE(ProcessGetRqst) {

    Mock<DaqDB::PmemPooler> poolerMock;
    Mock<DaqDB::RTree> rtreeMock;
    char valRef[] = "abc";
    size_t sizeRef = 3;

    When(OverloadedMethod(rtreeMock, Get,
                          DaqDB::StatusCode(const char *, int32_t, void **,
                                            size_t *, uint8_t *))
             .Using(expectedKey, expectedKeySize, _, _, _))
        .AlwaysDo([&](const char *key, int32_t keySize, void **val,
                      size_t *valSize, uint8_t *) {
            *val = valRef;
            *valSize = sizeRef;
            return DaqDB::StatusCode::Ok;
        });

    DaqDB::PmemPooler &pooler = poolerMock.get();
    DaqDB::RTreeEngine &rtree = rtreeMock.get();
    pooler.rtree.reset(&rtree);

    pooler.requests = new DaqDB::PmemRqst *[1];
    pooler.requests[0] =
        new DaqDB::PmemRqst(DaqDB::RqstOperation::GET, expectedKey,
                            expectedKeySize, nullptr, 0, nullptr);
    pooler.requestCount = 1;

    pooler.Process();

    Verify(OverloadedMethod(rtreeMock, Get,
                            DaqDB::StatusCode(const char *, int32_t, void **,
                                              size_t *, uint8_t *)))
        .Exactly(1);
    delete[] pooler.requests;
}

BOOST_AUTO_TEST_CASE(ProcessMultipleGetRqst) {

    Mock<DaqDB::PmemPooler> poolerMock;
    Mock<DaqDB::RTree> rtreeMock;
    char valRef[] = "abc";
    size_t sizeRef = 3;

    When(OverloadedMethod(rtreeMock, Get,
                          DaqDB::StatusCode(const char *, int32_t, void **,
                                            size_t *, uint8_t *))
             .Using(expectedKey, expectedKeySize, _, _, _))
        .AlwaysDo([&](const char *key, int32_t keySize, void **val,
                      size_t *valSize, uint8_t *) {
            *val = valRef;
            *valSize = sizeRef;
            return DaqDB::StatusCode::Ok;
        });

    DaqDB::PmemPooler &pooler = poolerMock.get();
    DaqDB::RTreeEngine &rtree = rtreeMock.get();
    pooler.rtree.reset(&rtree);

    pooler.requests = new DaqDB::PmemRqst *[DEQUEUE_RING_LIMIT];
    for (int index = 0; index < DEQUEUE_RING_LIMIT; index++) {
        pooler.requests[index] =
            new DaqDB::PmemRqst(DaqDB::RqstOperation::GET, expectedKey,
                                expectedKeySize, nullptr, 0, nullptr);
    }
    pooler.requestCount = DEQUEUE_RING_LIMIT;

    pooler.Process();

    Verify(OverloadedMethod(rtreeMock, Get,
                            DaqDB::StatusCode(const char *, int32_t, void **,
                                              size_t *, uint8_t *)))
        .Exactly(DEQUEUE_RING_LIMIT);
    delete[] pooler.requests;
}

BOOST_AUTO_TEST_CASE(ProcessPutTestCallback) {

    Mock<DaqDB::PmemPooler> poolerMock;
    Mock<DaqDB::RTree> rtreeMock;

    When(OverloadedMethod(
             rtreeMock, Put,
             DaqDB::StatusCode(const char *, int32_t, const char *, int32_t))
             .Using(expectedKey, expectedKeySize, expectedVal, expectedValSize))
        .Return(DaqDB::StatusCode::Ok)
        .Return(DaqDB::StatusCode::UnknownError);

    DaqDB::PmemPooler &pooler = poolerMock.get();
    DaqDB::RTreeEngine &rtree = rtreeMock.get();
    pooler.rtree.reset(&rtree);

    pooler.requests = new DaqDB::PmemRqst *[2];
    pooler.requests[0] = new DaqDB::PmemRqst(
        DaqDB::RqstOperation::PUT, expectedKey, expectedKeySize, expectedVal,
        expectedValSize,
        [&](DaqDB::KVStoreBase *kvs, DaqDB::Status status, const char *key,
            const size_t keySize, const char *value, const size_t valueSize) {
            BOOST_REQUIRE(status.ok());
            BOOST_CHECK_EQUAL(key, expectedKey);
            BOOST_CHECK_EQUAL(keySize, expectedKeySize);
            BOOST_CHECK_EQUAL(value, nullptr);
            BOOST_CHECK_EQUAL(valueSize, 0);
        });

    pooler.requests[1] = new DaqDB::PmemRqst(
        DaqDB::RqstOperation::PUT, expectedKey, expectedKeySize, expectedVal,
        expectedKeySize,
        [&](DaqDB::KVStoreBase *kvs, DaqDB::Status status, const char *key,
            const size_t keySize, const char *value, const size_t valueSize) {
            BOOST_REQUIRE(!status.ok());
            BOOST_CHECK_EQUAL(key, expectedKey);
            BOOST_CHECK_EQUAL(keySize, expectedKeySize);
            BOOST_CHECK_EQUAL(value, nullptr);
            BOOST_CHECK_EQUAL(valueSize, 0);
        });
    pooler.requestCount = 2;

    pooler.Process();

    Verify(OverloadedMethod(
               rtreeMock, Put,
               DaqDB::StatusCode(const char *, int32_t, const char *, int32_t)))
        .Exactly(2);
    delete[] pooler.requests;
}

BOOST_AUTO_TEST_CASE(ProcessGetTestCallback) {

    Mock<DaqDB::PmemPooler> poolerMock;
    Mock<DaqDB::RTree> rtreeMock;
    char valRef[] = "abc";
    size_t sizeRef = 3;

    When(OverloadedMethod(rtreeMock, Get,
                          DaqDB::StatusCode(const char *, int32_t, void **,
                                            size_t *, uint8_t *))
             .Using(expectedKey, expectedKeySize, _, _, _))
        .Do([&](const char *key, int32_t keySize, void **val, size_t *valSize,
                uint8_t *) {
            *val = valRef;
            *valSize = sizeRef;
            return DaqDB::StatusCode::Ok;
        })
        .Do([&](const char *key, int32_t keySize, void **val, size_t *valSize,
                uint8_t *) {
            *val = valRef;
            *valSize = sizeRef;
            return DaqDB::StatusCode::KeyNotFound;
        });

    DaqDB::PmemPooler &pooler = poolerMock.get();
    DaqDB::RTreeEngine &rtree = rtreeMock.get();
    pooler.rtree.reset(&rtree);

    pooler.requests = new DaqDB::PmemRqst *[2];
    pooler.requests[0] = new DaqDB::PmemRqst(
        DaqDB::RqstOperation::GET, expectedKey, expectedKeySize, nullptr, 0,
        [&](DaqDB::KVStoreBase *kvs, DaqDB::Status status, const char *key,
            const size_t keySize, const char *value, const size_t valueSize) {
            BOOST_REQUIRE(status.ok());
            BOOST_CHECK_EQUAL(key, expectedKey);
            BOOST_CHECK_EQUAL(keySize, expectedKeySize);
        });

    pooler.requests[1] = new DaqDB::PmemRqst(
        DaqDB::RqstOperation::GET, expectedKey, expectedKeySize, nullptr, 0,
        [&](DaqDB::KVStoreBase *kvs, DaqDB::Status status, const char *key,
            const size_t keySize, const char *value, const size_t valueSize) {
            BOOST_REQUIRE(!status.ok());
            BOOST_CHECK_EQUAL(key, expectedKey);
            BOOST_CHECK_EQUAL(keySize, expectedKeySize);
        });
    pooler.requestCount = 2;

    pooler.Process();

    Verify(OverloadedMethod(rtreeMock, Get,
                            DaqDB::StatusCode(const char *, int32_t, void **,
                                              size_t *, uint8_t *)))
        .Exactly(2);
    delete[] pooler.requests;
}
