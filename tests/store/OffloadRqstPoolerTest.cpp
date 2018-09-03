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

#include "../../lib/store/OffloadRqstPooler.cpp"
#include "../../lib/store/RTree.h"

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
    Mock<DaqDB::OffloadRqstPooler> poolerMock;
    Mock<DaqDB::RTree> rtreeMock;
    char valRef[] = "abc";
    size_t sizeRef = 3;
    uint8_t location = DISK;

    DaqDB::OffloadRqstPooler &pooler = poolerMock.get();
    When(OverloadedMethod(rtreeMock, Get,
                          DaqDB::StatusCode(const char *, int32_t, void **,
                                            size_t *, uint8_t *))
             .Using(expectedKey, expectedKeySize, _, _, _))
        .AlwaysDo([&](const char *key, int32_t keySize, void **val,
                      size_t *valSize, uint8_t *loc) {
            *val = valRef;
            *valSize = sizeRef;
            *loc = location;
            return DaqDB::StatusCode::Ok;
        });
    When(Method(poolerMock, Read)).Return(0);
    When(Method(poolerMock, Write)).Return(0);

    pooler.ProcessMsg();
    VerifyNoOtherInvocations(
        OverloadedMethod(rtreeMock, Get,
                         DaqDB::StatusCode(const char *, int32_t, void **,
                                           size_t *, uint8_t *)));
    VerifyNoOtherInvocations(Method(poolerMock, Read));
    VerifyNoOtherInvocations(Method(poolerMock, Write));
}

BOOST_AUTO_TEST_CASE(ProcessGetRequest) {
    Mock<DaqDB::OffloadRqstPooler> poolerMock;
    Mock<DaqDB::RTree> rtreeMock;
    uint64_t lbaRef = 123;
    size_t valSizeRef = 4;
    char valRef[] = "1234";
    uint8_t location = DISK;

    DaqDB::OffloadRqstPooler &pooler = poolerMock.get();
    When(OverloadedMethod(rtreeMock, Get,
                          DaqDB::StatusCode(const char *, int32_t, void **,
                                            size_t *, uint8_t *))
             .Using(expectedKey, expectedKeySize, _, _, _))
        .AlwaysDo([&](const char *key, int32_t keySize, void **val,
                      size_t *valSize, uint8_t *loc) {
            *val = &lbaRef;
            *valSize = valSizeRef;
            *loc = location;
            return DaqDB::StatusCode::Ok;
        });
    When(Method(poolerMock, Read)).Return(0);
    When(Method(poolerMock, Write)).Return(0);

    DaqDB::RTreeEngine &rtree = rtreeMock.get();
    pooler.rtree.reset(&rtree);
    DaqDB::BdevContext bdvCtx;
    bdvCtx.blk_size = 512;
    bdvCtx.blk_num = 1024;
    bdvCtx.buf_align = 1;
    pooler.SetBdevContext(bdvCtx);

    pooler.processArray[0] =
        new DaqDB::OffloadRqstMsg(DaqDB::OffloadRqstOperation::GET, expectedKey,
                                  expectedKeySize, nullptr, 0, nullptr);
    pooler.processArrayCount = 1;

    pooler.ProcessMsg();
    VerifyNoOtherInvocations(Method(poolerMock, Write));

    Verify(OverloadedMethod(rtreeMock, Get,
                            DaqDB::StatusCode(const char *, int32_t, void **,
                                              size_t *, uint8_t *)))
        .Exactly(1);
    Verify(Method(poolerMock, Read)).Exactly(1);
}

BOOST_AUTO_TEST_CASE(ProcessUpdateRequest) {
    Mock<DaqDB::OffloadRqstPooler> poolerMock;
    Mock<DaqDB::RTree> rtreeMock;

    uint64_t lbaRef = 123;
    size_t valSizeRef = 4;
    char valRef[] = "1234";
    uint8_t location = PMEM;

    DaqDB::OffloadRqstPooler &pooler = poolerMock.get();
    When(OverloadedMethod(rtreeMock, Get,
                          DaqDB::StatusCode(const char *, int32_t, void **,
                                            size_t *, uint8_t *))
             .Using(expectedKey, expectedKeySize, _, _, _))
        .AlwaysDo([&](const char *key, int32_t keySize, void **val,
                      size_t *valSize, uint8_t *loc) {
            *val = valRef;
            *valSize = valSizeRef;
            *loc = location;
            return DaqDB::StatusCode::Ok;
        });
    When(Method(rtreeMock, AllocateIOVForKey))
        .AlwaysDo([&](const char *key, uint64_t **ptr, size_t size) {
            *ptr = &lbaRef;
            return DaqDB::StatusCode::Ok;
        });

    When(Method(poolerMock, GetFreeLba)).Return(1);
    When(Method(poolerMock, Read)).Return(0);
    When(Method(poolerMock, Write)).Return(0);

    DaqDB::RTreeEngine &rtree = rtreeMock.get();
    pooler.rtree.reset(&rtree);
    DaqDB::BdevContext bdvCtx;
    bdvCtx.blk_size = 512;
    bdvCtx.blk_num = 1024;
    bdvCtx.buf_align = 1;
    pooler.SetBdevContext(bdvCtx);

    pooler.processArray[0] = new DaqDB::OffloadRqstMsg(
        DaqDB::OffloadRqstOperation::UPDATE, expectedKey, expectedKeySize,
        nullptr, 0, nullptr);
    pooler.processArrayCount = 1;

    pooler.ProcessMsg();
    VerifyNoOtherInvocations(Method(poolerMock, Read));

    Verify(OverloadedMethod(rtreeMock, Get,
                            DaqDB::StatusCode(const char *, int32_t, void **,
                                              size_t *, uint8_t *)))
        .Exactly(1);
    Verify(Method(poolerMock, Write)).Exactly(1);
}

BOOST_AUTO_TEST_CASE(ProcessRemoveRequest) {
    Mock<DaqDB::OffloadRqstPooler> poolerMock;
    Mock<DaqDB::RTree> rtreeMock;
    uint64_t lbaRef = 123;
    size_t valSizeRef = 4;
    char valRef[] = "1234";
    uint8_t location = PMEM;

    DaqDB::OffloadRqstPooler &pooler = poolerMock.get();
    When(OverloadedMethod(rtreeMock, Get,
                          DaqDB::StatusCode(const char *, int32_t, void **,
                                            size_t *, uint8_t *))
             .Using(expectedKey, expectedKeySize, _, _, _))
        .AlwaysDo([&](const char *key, int32_t keySize, void **val,
                      size_t *valSize, uint8_t *loc) {
            *val = valRef;
            *valSize = valSizeRef;
            *loc = location;
            return DaqDB::StatusCode::Ok;
        });
    When(Method(poolerMock, Read)).Return(0);
    When(Method(poolerMock, Write)).Return(0);

    DaqDB::RTreeEngine &rtree = rtreeMock.get();
    pooler.rtree.reset(&rtree);
    DaqDB::BdevContext bdvCtx;
    bdvCtx.blk_size = 512;
    bdvCtx.blk_num = 1024;
    bdvCtx.buf_align = 1;
    pooler.SetBdevContext(bdvCtx);

    pooler.processArray[0] = new DaqDB::OffloadRqstMsg(
        DaqDB::OffloadRqstOperation::REMOVE, expectedKey, expectedKeySize,
        nullptr, 0, nullptr);
    pooler.processArrayCount = 1;

    pooler.ProcessMsg();
    VerifyNoOtherInvocations(Method(poolerMock, Read));
    VerifyNoOtherInvocations(Method(poolerMock, Write));
    Verify(OverloadedMethod(rtreeMock, Get,
                            DaqDB::StatusCode(const char *, int32_t, void **,
                                              size_t *, uint8_t *)))
        .Exactly(1);
}
