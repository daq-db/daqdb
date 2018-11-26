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

#include "../../../lib/pmem/PmemPoller.cpp"
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
    Mock<DaqDB::PmemPoller> pollerMock;
    Mock<DaqDB::RTree> rtreeMock;

    When(OverloadedMethod(
             rtreeMock, Put,
             DaqDB::StatusCode(const char *, int32_t, const char *, int32_t)))
        .Return(DaqDB::StatusCode::OK);

    DaqDB::PmemPoller &poller = pollerMock.get();
    DaqDB::RTreeEngine &rtree = rtreeMock.get();
    poller.rtree.reset(&rtree);

    poller.process();
    VerifyNoOtherInvocations(OverloadedMethod(
        rtreeMock, Put,
        DaqDB::StatusCode(const char *, int32_t, const char *, int32_t)));
}

BOOST_AUTO_TEST_CASE(ProcessPutRqst) {

    Mock<DaqDB::PmemPoller> pollerMock;
    Mock<DaqDB::RTree> rtreeMock;

    When(OverloadedMethod(
             rtreeMock, Put,
             DaqDB::StatusCode(const char *, int32_t, const char *, int32_t))
             .Using(expectedKey, expectedKeySize, expectedVal, expectedValSize))
        .Return(DaqDB::StatusCode::OK);

    DaqDB::PmemPoller &poller = pollerMock.get();
    DaqDB::RTreeEngine &rtree = rtreeMock.get();
    poller.rtree.reset(&rtree);

    poller.requests = new DaqDB::PmemRqst *[1];
    poller.requests[0] = new DaqDB::PmemRqst(
        DaqDB::RqstOperation::PUT, expectedKey, expectedKeySize, expectedVal,
        expectedValSize, nullptr);
    poller.requestCount = 1;

    poller.process();

    Verify(OverloadedMethod(
               rtreeMock, Put,
               DaqDB::StatusCode(const char *, int32_t, const char *, int32_t)))
        .Exactly(1);

    delete[] poller.requests;
}

BOOST_AUTO_TEST_CASE(ProcessMultiplePutRqst) {

    Mock<DaqDB::PmemPoller> pollerMock;
    Mock<DaqDB::RTree> rtreeMock;

    When(OverloadedMethod(
             rtreeMock, Put,
             DaqDB::StatusCode(const char *, int32_t, const char *, int32_t))
             .Using(expectedKey, expectedKeySize, expectedVal, expectedValSize))
        .AlwaysReturn(DaqDB::StatusCode::OK);

    DaqDB::PmemPoller &poller = pollerMock.get();
    DaqDB::RTreeEngine &rtree = rtreeMock.get();
    poller.rtree.reset(&rtree);

    poller.requests = new DaqDB::PmemRqst *[DEQUEUE_RING_LIMIT];
    for (int index = 0; index < DEQUEUE_RING_LIMIT; index++) {
        poller.requests[index] = new DaqDB::PmemRqst(
            DaqDB::RqstOperation::PUT, expectedKey, expectedKeySize,
            expectedVal, expectedValSize, nullptr);
    }
    poller.requestCount = DEQUEUE_RING_LIMIT;

    poller.process();

    Verify(OverloadedMethod(
               rtreeMock, Put,
               DaqDB::StatusCode(const char *, int32_t, const char *, int32_t)))
        .Exactly(DEQUEUE_RING_LIMIT);
    delete[] poller.requests;
}

BOOST_AUTO_TEST_CASE(ProcessGetRqst) {

    Mock<DaqDB::PmemPoller> pollerMock;
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
            return DaqDB::StatusCode::OK;
        });

    DaqDB::PmemPoller &poller = pollerMock.get();
    DaqDB::RTreeEngine &rtree = rtreeMock.get();
    poller.rtree.reset(&rtree);

    poller.requests = new DaqDB::PmemRqst *[1];
    poller.requests[0] =
        new DaqDB::PmemRqst(DaqDB::RqstOperation::GET, expectedKey,
                            expectedKeySize, nullptr, 0, nullptr);
    poller.requestCount = 1;

    poller.process();

    Verify(OverloadedMethod(rtreeMock, Get,
                            DaqDB::StatusCode(const char *, int32_t, void **,
                                              size_t *, uint8_t *)))
        .Exactly(1);
    delete[] poller.requests;
}

BOOST_AUTO_TEST_CASE(ProcessMultipleGetRqst) {

    Mock<DaqDB::PmemPoller> pollerMock;
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
            return DaqDB::StatusCode::OK;
        });

    DaqDB::PmemPoller &poller = pollerMock.get();
    DaqDB::RTreeEngine &rtree = rtreeMock.get();
    poller.rtree.reset(&rtree);

    poller.requests = new DaqDB::PmemRqst *[DEQUEUE_RING_LIMIT];
    for (int index = 0; index < DEQUEUE_RING_LIMIT; index++) {
        poller.requests[index] =
            new DaqDB::PmemRqst(DaqDB::RqstOperation::GET, expectedKey,
                                expectedKeySize, nullptr, 0, nullptr);
    }
    poller.requestCount = DEQUEUE_RING_LIMIT;

    poller.process();

    Verify(OverloadedMethod(rtreeMock, Get,
                            DaqDB::StatusCode(const char *, int32_t, void **,
                                              size_t *, uint8_t *)))
        .Exactly(DEQUEUE_RING_LIMIT);
    delete[] poller.requests;
}

BOOST_AUTO_TEST_CASE(ProcessPutTestCallback) {

    Mock<DaqDB::PmemPoller> pollerMock;
    Mock<DaqDB::RTree> rtreeMock;

    When(OverloadedMethod(
             rtreeMock, Put,
             DaqDB::StatusCode(const char *, int32_t, const char *, int32_t))
             .Using(expectedKey, expectedKeySize, expectedVal, expectedValSize))
        .Return(DaqDB::StatusCode::OK)
        .Return(DaqDB::StatusCode::UNKNOWN_ERROR);

    DaqDB::PmemPoller &poller = pollerMock.get();
    DaqDB::RTreeEngine &rtree = rtreeMock.get();
    poller.rtree.reset(&rtree);

    poller.requests = new DaqDB::PmemRqst *[2];
    poller.requests[0] = new DaqDB::PmemRqst(
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

    poller.requests[1] = new DaqDB::PmemRqst(
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
    poller.requestCount = 2;

    poller.process();

    Verify(OverloadedMethod(
               rtreeMock, Put,
               DaqDB::StatusCode(const char *, int32_t, const char *, int32_t)))
        .Exactly(2);
    delete[] poller.requests;
}

BOOST_AUTO_TEST_CASE(ProcessGetTestCallback) {

    Mock<DaqDB::PmemPoller> pollerMock;
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
            return DaqDB::StatusCode::OK;
        })
        .Do([&](const char *key, int32_t keySize, void **val, size_t *valSize,
                uint8_t *) {
            *val = valRef;
            *valSize = sizeRef;
            return DaqDB::StatusCode::KEY_NOT_FOUND;
        });

    DaqDB::PmemPoller &poller = pollerMock.get();
    DaqDB::RTreeEngine &rtree = rtreeMock.get();
    poller.rtree.reset(&rtree);

    poller.requests = new DaqDB::PmemRqst *[2];
    poller.requests[0] = new DaqDB::PmemRqst(
        DaqDB::RqstOperation::GET, expectedKey, expectedKeySize, nullptr, 0,
        [&](DaqDB::KVStoreBase *kvs, DaqDB::Status status, const char *key,
            const size_t keySize, const char *value, const size_t valueSize) {
            BOOST_REQUIRE(status.ok());
            BOOST_CHECK_EQUAL(key, expectedKey);
            BOOST_CHECK_EQUAL(keySize, expectedKeySize);
        });

    poller.requests[1] = new DaqDB::PmemRqst(
        DaqDB::RqstOperation::GET, expectedKey, expectedKeySize, nullptr, 0,
        [&](DaqDB::KVStoreBase *kvs, DaqDB::Status status, const char *key,
            const size_t keySize, const char *value, const size_t valueSize) {
            BOOST_REQUIRE(!status.ok());
            BOOST_CHECK_EQUAL(key, expectedKey);
            BOOST_CHECK_EQUAL(keySize, expectedKeySize);
        });
    poller.requestCount = 2;

    poller.process();

    Verify(OverloadedMethod(rtreeMock, Get,
                            DaqDB::StatusCode(const char *, int32_t, void **,
                                              size_t *, uint8_t *)))
        .Exactly(2);
    delete[] poller.requests;
}
