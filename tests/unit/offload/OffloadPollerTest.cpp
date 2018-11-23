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

#include "../../lib/offload/OffloadPoller.cpp"
#include "../../lib/pmem/RTree.h"
#include "../../lib/spdk/SpdkBdev.h"
#include "../../lib/spdk/SpdkCore.h"

#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>

#include <fakeit.hpp>
#include <iostream>
#include <stdlib.h>

namespace ut = boost::unit_test;

using namespace fakeit;

static const char *expectedKey = "key123";
static const size_t expectedKeySize = 6;

#define BOOST_TEST_DETECT_MEMORY_LEAK 1

void *spdk_zmalloc(size_t size, size_t align, uint64_t *phys_addr,
                   int socket_id, uint32_t flags) {
    return malloc(size);
}

BOOST_AUTO_TEST_CASE(ProcessEmptyRing) {
    Mock<DaqDB::OffloadPoller> pollerMock;
    Mock<DaqDB::RTree> rtreeMock;
    char valRef[] = "abc";
    size_t sizeRef = 3;
    uint8_t location = DISK;

    DaqDB::OffloadPoller &poller = pollerMock.get();
    When(OverloadedMethod(rtreeMock, Get,
                          DaqDB::StatusCode(const char *, int32_t, void **,
                                            size_t *, uint8_t *))
             .Using(expectedKey, expectedKeySize, _, _, _))
        .AlwaysDo([&](const char *key, int32_t keySize, void **val,
                      size_t *valSize, uint8_t *loc) {
            *val = valRef;
            *valSize = sizeRef;
            *loc = location;
            return DaqDB::StatusCode::OK;
        });
    When(Method(pollerMock, read)).Return(0);
    When(Method(pollerMock, write)).Return(0);

    poller.process();
    VerifyNoOtherInvocations(OverloadedMethod(
        rtreeMock, Get, DaqDB::StatusCode(const char *, int32_t, void **,
                                          size_t *, uint8_t *)));
    VerifyNoOtherInvocations(Method(pollerMock, read));
    VerifyNoOtherInvocations(Method(pollerMock, write));
}

BOOST_AUTO_TEST_CASE(ProcessGetRequest) {
    Mock<DaqDB::OffloadPoller> pollerMock;
    Mock<DaqDB::RTree> rtreeMock;

    uint64_t lbaRef = 123;
    size_t valSizeRef = 4;
    char valRef[] = "1234";
    uint8_t location = DISK;

    DaqDB::SpdkBdev spdkBdev;
    spdkBdev.spBdevCtx.reset(new DaqDB::SpdkBdevCtx());
    spdkBdev.spBdevCtx->blk_size = 512;
    spdkBdev.spBdevCtx->blk_num = 1024;
    spdkBdev.spBdevCtx->buf_align = 1;

    DaqDB::OffloadPoller &poller = pollerMock.get();
    When(OverloadedMethod(rtreeMock, Get,
                          DaqDB::StatusCode(const char *, int32_t, void **,
                                            size_t *, uint8_t *))
             .Using(expectedKey, expectedKeySize, _, _, _))
        .AlwaysDo([&](const char *key, int32_t keySize, void **val,
                      size_t *valSize, uint8_t *loc) {
            *val = &lbaRef;
            *valSize = valSizeRef;
            *loc = location;
            return DaqDB::StatusCode::OK;
        });
    When(Method(pollerMock, read)).Return(0);
    When(Method(pollerMock, write)).Return(0);
    When(Method(pollerMock, getBdev)).AlwaysReturn(&spdkBdev);
    When(Method(pollerMock, getBdevCtx)).AlwaysReturn(spdkBdev.spBdevCtx.get());

    DaqDB::RTreeEngine &rtree = rtreeMock.get();
    poller.rtree = &rtree;

    poller.requests = new DaqDB::OffloadRqst *[1];
    poller.requests[0] =
        new DaqDB::OffloadRqst(DaqDB::OffloadOperation::GET, expectedKey,
                               expectedKeySize, nullptr, 0, nullptr);
    poller.requestCount = 1;

    poller.process();

    VerifyNoOtherInvocations(Method(pollerMock, write));
    Verify(OverloadedMethod(rtreeMock, Get,
                            DaqDB::StatusCode(const char *, int32_t, void **,
                                              size_t *, uint8_t *)))
        .Exactly(1);
    Verify(Method(pollerMock, read)).Exactly(1);

    delete[] poller.requests;
}

BOOST_AUTO_TEST_CASE(ProcessUpdateRequest) {
    Mock<DaqDB::OffloadPoller> pollerMock;
    Mock<DaqDB::RTree> rtreeMock;

    uint64_t lbaRef = 123;
    size_t valSizeRef = 4;
    char valRef[] = "1234";
    uint8_t location = PMEM;

    DaqDB::SpdkBdev spdkBdev;
    spdkBdev.spBdevCtx.reset(new DaqDB::SpdkBdevCtx());
    spdkBdev.spBdevCtx->blk_size = 512;
    spdkBdev.spBdevCtx->blk_num = 1024;
    spdkBdev.spBdevCtx->buf_align = 1;

    DaqDB::OffloadPoller &poller = pollerMock.get();
    When(OverloadedMethod(rtreeMock, Get,
                          DaqDB::StatusCode(const char *, int32_t, void **,
                                            size_t *, uint8_t *))
             .Using(expectedKey, expectedKeySize, _, _, _))
        .AlwaysDo([&](const char *key, int32_t keySize, void **val,
                      size_t *valSize, uint8_t *loc) {
            *val = valRef;
            *valSize = valSizeRef;
            *loc = location;
            return DaqDB::StatusCode::OK;
        });
    When(Method(rtreeMock, AllocateIOVForKey))
        .AlwaysDo([&](const char *key, uint64_t **ptr, size_t size) {
            *ptr = &lbaRef;
            return DaqDB::StatusCode::OK;
        });

    When(Method(pollerMock, getFreeLba)).Return(1);
    When(Method(pollerMock, read)).Return(0);
    When(Method(pollerMock, write)).Return(0);
    When(Method(pollerMock, getBdev)).AlwaysReturn(&spdkBdev);
    When(Method(pollerMock, getBdevCtx)).AlwaysReturn(spdkBdev.spBdevCtx.get());

    DaqDB::RTreeEngine &rtree = rtreeMock.get();
    poller.rtree = &rtree;

    poller.requests = new DaqDB::OffloadRqst *[1];
    poller.requests[0] =
        new DaqDB::OffloadRqst(DaqDB::OffloadOperation::UPDATE, expectedKey,
                               expectedKeySize, nullptr, 0, nullptr);
    poller.requestCount = 1;

    poller.process();
    VerifyNoOtherInvocations(Method(pollerMock, read));

    Verify(OverloadedMethod(rtreeMock, Get,
                            DaqDB::StatusCode(const char *, int32_t, void **,
                                              size_t *, uint8_t *)))
        .Exactly(1);
    Verify(Method(pollerMock, write)).Exactly(1);
    delete[] poller.requests;
}

BOOST_AUTO_TEST_CASE(ProcessRemoveRequest) {
    Mock<DaqDB::OffloadPoller> pollerMock;
    Mock<DaqDB::RTree> rtreeMock;
    uint64_t lbaRef = 123;
    size_t valSizeRef = 4;
    char valRef[] = "1234";
    uint8_t location = PMEM;

    DaqDB::OffloadPoller &poller = pollerMock.get();
    When(OverloadedMethod(rtreeMock, Get,
                          DaqDB::StatusCode(const char *, int32_t, void **,
                                            size_t *, uint8_t *))
             .Using(expectedKey, expectedKeySize, _, _, _))
        .AlwaysDo([&](const char *key, int32_t keySize, void **val,
                      size_t *valSize, uint8_t *loc) {
            *val = valRef;
            *valSize = valSizeRef;
            *loc = location;

            return DaqDB::StatusCode::OK;
        });
    When(Method(pollerMock, read)).Return(0);
    When(Method(pollerMock, write)).Return(0);

    DaqDB::RTreeEngine &rtree = rtreeMock.get();
    poller.rtree = &rtree;

    poller.requests = new DaqDB::OffloadRqst *[1];
    poller.requests[0] =
        new DaqDB::OffloadRqst(DaqDB::OffloadOperation::REMOVE, expectedKey,
                               expectedKeySize, nullptr, 0, nullptr);
    poller.requestCount = 1;

    poller.process();
    VerifyNoOtherInvocations(Method(pollerMock, read));
    VerifyNoOtherInvocations(Method(pollerMock, write));
    Verify(OverloadedMethod(rtreeMock, Get,
                            DaqDB::StatusCode(const char *, int32_t, void **,
                                              size_t *, uint8_t *)))
        .Exactly(1);
    delete[] poller.requests;
}
