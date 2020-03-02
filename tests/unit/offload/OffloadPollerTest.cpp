/**
 *  Copyright (c) 2019 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License. 
 */

#include <cstdint>

#include "../../lib/offload/OffloadPoller.cpp"
#include "../../lib/pmem/ARTree.h"
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
    Mock<DaqDB::SpdkBdev> bdevMock;
    Mock<DaqDB::ARTree> rtreeMock;
    char valRef[] = "abc";
    size_t sizeRef = 3;
    uint8_t location = DISK;

    DaqDB::OffloadPoller &poller = pollerMock.get();
    When(OverloadedMethod(rtreeMock, Get, void(const char *, int32_t, void **,
                                               size_t *, uint8_t *))
             .Using(expectedKey, expectedKeySize, _, _, _))
        .AlwaysDo([&](const char *key, int32_t keySize, void **val,
                      size_t *valSize, uint8_t *loc) {
            *val = valRef;
            *valSize = sizeRef;
            *loc = location;
        });
    When(Method(bdevMock, read)).Return(0);
    When(Method(bdevMock, write)).Return(0);

    poller.process();
    VerifyNoOtherInvocations(
        OverloadedMethod(rtreeMock, Get, void(const char *, int32_t, void **,
                                              size_t *, uint8_t *)));
    VerifyNoOtherInvocations(Method(bdevMock, read));
    VerifyNoOtherInvocations(Method(bdevMock, write));
}

BOOST_AUTO_TEST_CASE(ProcessGetRequest) {
    Mock<DaqDB::OffloadPoller> pollerMock;
    Mock<DaqDB::ARTree> rtreeMock;

    uint64_t lbaRef = 123;
    struct DaqDB::DeviceAddr dAddr;
    dAddr.lba = 123;
    dAddr.busAddr.busAddr = 0;
    size_t valSizeRef = sizeof(struct DaqDB::DeviceAddr);

    char valRef[] = "1234";
    uint8_t location = DISK;

    Mock<DaqDB::SpdkBdev> bdevMock;
    DaqDB::SpdkBdev &spdkBdev = bdevMock.get();

    spdkBdev.spBdevCtx.blk_size = 512;
    spdkBdev.spBdevCtx.blk_num = 1024;
    spdkBdev.spBdevCtx.buf_align = 1;
    spdkBdev.spBdevCtx.io_min_size = 4096;
    spdkBdev.spBdevCtx.bdev = 0;
    spdkBdev.spBdevCtx.bdev_desc = 0;
    spdkBdev.spBdevCtx.io_channel = 0;
    spdkBdev.spBdevCtx.buff = 0;
    strcpy(spdkBdev.spBdevCtx.bdev_name, "test");
    strcpy(spdkBdev.spBdevCtx.bdev_addr, "test");
    spdkBdev.spBdevCtx.data_blk_size = 0;
    spdkBdev.spBdevCtx.io_pool_size = 0;
    spdkBdev.spBdevCtx.io_cache_size = 0;
    spdkBdev.spBdevCtx.io_min_size = 4096;
    spdkBdev.spBdevCtx.state = DaqDB::CSpdkBdevState::SPDK_BDEV_READY;

    spdkBdev.blkNumForLba = 0;
    spdkBdev.IoBytesQueued = 0;
    spdkBdev.IoBytesMaxQueued = 0;

    DaqDB::OffloadPoller &poller = pollerMock.get();
    When(OverloadedMethod(
             rtreeMock, Get,
             void(const char *, int32_t, void **, size_t *, uint8_t *))
             .Using(expectedKey, expectedKeySize, _, _, _))
        .AlwaysDo([&](const char *key, int32_t keySize, void **val,
                      size_t *valSize, uint8_t *loc) {
            *val = &dAddr;
            *valSize = valSizeRef;
            *loc = location;
        });

    When(Method(bdevMock, getAlignedSize)).Return(0);
    When(Method(bdevMock, getOptimalSize)).Return(0);
    When(Method(bdevMock, getSizeInBlk)).Return(0);

    When(Method(bdevMock, read)).Return(0);
    When(Method(bdevMock, write)).Return(0);
    When(Method(pollerMock, getBdev)).AlwaysReturn(&spdkBdev);
    When(Method(pollerMock, getBdevCtx)).AlwaysReturn(&spdkBdev.spBdevCtx);

    DaqDB::RTreeEngine &rtree = rtreeMock.get();
    poller.rtree = &rtree;

    poller.requests = new DaqDB::OffloadRqst *[1];
    poller.requests[0] = DaqDB::OffloadRqst::getPool.get();
    poller.requests[0]->finalizeGet(expectedKey, expectedKeySize, nullptr, 0,
                                    nullptr);
    poller.requestCount = 1;

    poller.process();

    VerifyNoOtherInvocations(Method(bdevMock, write));
    Verify(OverloadedMethod(
               rtreeMock, Get,
               void(const char *, int32_t, void **, size_t *, uint8_t *)))
        .Exactly(1);
    Verify(Method(bdevMock, read)).Exactly(1);

    DaqDB::OffloadRqst::getPool.put(poller.requests[0]);
    delete[] poller.requests;
}

BOOST_AUTO_TEST_CASE(ProcessUpdateRequest) {
    Mock<DaqDB::OffloadPoller> pollerMock;
    Mock<DaqDB::ARTree> rtreeMock;

    struct DaqDB::DeviceAddr dRef;
    dRef.lba = 123;
    dRef.busAddr.busAddr = 0;
    size_t valSizeRef = sizeof(struct DaqDB::DeviceAddr);

    struct DaqDB::PciAddr pciAddrRef = {0, 0, 0, 0};
    char valRef[] = "1234";
    uint8_t location = PMEM;

    Mock<DaqDB::SpdkBdev> bdevMock;
    DaqDB::SpdkBdev &spdkBdev = bdevMock.get();
    spdkBdev.spBdevCtx.blk_size = 512;
    spdkBdev.spBdevCtx.blk_num = 1024;
    spdkBdev.spBdevCtx.buf_align = 1;
    spdkBdev.spBdevCtx.io_min_size = 4096;
    spdkBdev.spBdevCtx.bdev = 0;
    spdkBdev.spBdevCtx.bdev_desc = 0;
    spdkBdev.spBdevCtx.io_channel = 0;
    spdkBdev.spBdevCtx.buff = 0;
    strcpy(spdkBdev.spBdevCtx.bdev_name, "test");
    strcpy(spdkBdev.spBdevCtx.bdev_addr, "test");
    spdkBdev.spBdevCtx.data_blk_size = 0;
    spdkBdev.spBdevCtx.io_pool_size = 0;
    spdkBdev.spBdevCtx.io_cache_size = 0;
    spdkBdev.spBdevCtx.io_min_size = 4096;
    spdkBdev.spBdevCtx.state = DaqDB::CSpdkBdevState::SPDK_BDEV_READY;

    spdkBdev.blkNumForLba = 0;
    spdkBdev.IoBytesQueued = 0;
    spdkBdev.IoBytesMaxQueued = 0;

    DaqDB::OffloadPoller &poller = pollerMock.get();
    When(OverloadedMethod(
             rtreeMock, Get,
             void(const char *, int32_t, void **, size_t *, uint8_t *))
             .Using(expectedKey, expectedKeySize, _, _, _))
        .AlwaysDo([&](const char *key, int32_t keySize, void **val,
                      size_t *valSize, uint8_t *loc) {
            *val = &dRef;
            *valSize = valSizeRef;
            *loc = location;
        });
    When(Method(rtreeMock, AllocateAndUpdateValueWrapper))
        .AlwaysDo([&](const char *key, size_t size, const DaqDB::DeviceAddr *devAddr) {
            const_cast<DaqDB::DeviceAddr *>(devAddr)->lba = 123;
            const_cast<DaqDB::DeviceAddr *>(devAddr)->busAddr.pciAddr = pciAddrRef;
        });

    When(Method(bdevMock, getAlignedSize)).Return(0);
    When(Method(bdevMock, getOptimalSize)).Return(0);
    When(Method(bdevMock, getSizeInBlk)).Return(0);

    When(Method(pollerMock, getFreeLba)).Return(1);
    When(Method(bdevMock, read)).Return(0);
    When(Method(bdevMock, write)).Return(0);
    When(Method(pollerMock, getBdev)).AlwaysReturn(&spdkBdev);
    When(Method(pollerMock, getBdevCtx)).AlwaysReturn(&spdkBdev.spBdevCtx);

    DaqDB::RTreeEngine &rtree = rtreeMock.get();
    poller.rtree = &rtree;

    poller.requests = new DaqDB::OffloadRqst *[1];
    poller.requests[0] = DaqDB::OffloadRqst::updatePool.get();
    poller.requests[0]->finalizeUpdate(expectedKey, expectedKeySize, nullptr, 0,
                                       nullptr, PMEM);
    poller.requestCount = 1;

    poller.process();
    VerifyNoOtherInvocations(Method(bdevMock, read));

    VerifyNoOtherInvocations(OverloadedMethod(
        rtreeMock, Get,
        void(const char *, int32_t, void **, size_t *, uint8_t *)));
    Verify(Method(bdevMock, write)).Exactly(1);

    DaqDB::OffloadRqst::updatePool.put(poller.requests[0]);
    delete[] poller.requests;
}

BOOST_AUTO_TEST_CASE(ProcessRemoveRequest) {
    Mock<DaqDB::OffloadPoller> pollerMock;
    Mock<DaqDB::ARTree> rtreeMock;
    uint64_t lbaRef = 123;
    size_t valSizeRef = 4;
    char valRef[] = "1234";
    uint8_t location = PMEM;

    DaqDB::OffloadPoller &poller = pollerMock.get();
    When(OverloadedMethod(rtreeMock, Get, void(const char *, int32_t, void **,
                                               size_t *, uint8_t *))
             .Using(expectedKey, expectedKeySize, _, _, _))
        .AlwaysDo([&](const char *key, int32_t keySize, void **val,
                      size_t *valSize, uint8_t *loc) {
            *val = valRef;
            *valSize = valSizeRef;
            *loc = location;
        });

    Mock<DaqDB::SpdkBdev> bdevMock;
    DaqDB::SpdkBdev &spdkBdev = bdevMock.get();
    spdkBdev.spBdevCtx.blk_size = 512;
    spdkBdev.spBdevCtx.blk_num = 1024;
    spdkBdev.spBdevCtx.buf_align = 1;
    spdkBdev.spBdevCtx.io_min_size = 4096;
    spdkBdev.spBdevCtx.bdev = 0;
    spdkBdev.spBdevCtx.bdev_desc = 0;
    spdkBdev.spBdevCtx.io_channel = 0;
    spdkBdev.spBdevCtx.buff = 0;
    strcpy(spdkBdev.spBdevCtx.bdev_name, "test");
    strcpy(spdkBdev.spBdevCtx.bdev_addr, "test");
    spdkBdev.spBdevCtx.data_blk_size = 0;
    spdkBdev.spBdevCtx.io_pool_size = 0;
    spdkBdev.spBdevCtx.io_cache_size = 0;
    spdkBdev.spBdevCtx.io_min_size = 4096;
    spdkBdev.spBdevCtx.state = DaqDB::CSpdkBdevState::SPDK_BDEV_READY;

    spdkBdev.blkNumForLba = 0;
    spdkBdev.IoBytesQueued = 0;
    spdkBdev.IoBytesMaxQueued = 0;

    When(Method(bdevMock, read)).Return(0);
    When(Method(bdevMock, write)).Return(0);

    DaqDB::RTreeEngine &rtree = rtreeMock.get();
    poller.rtree = &rtree;

    poller.requests = new DaqDB::OffloadRqst *[1];
    poller.requests[0] = DaqDB::OffloadRqst::removePool.get();
    poller.requests[0]->finalizeRemove(expectedKey, expectedKeySize, nullptr, 0,
                                       nullptr);
    poller.requestCount = 1;

    poller.process();
    VerifyNoOtherInvocations(Method(bdevMock, read));
    VerifyNoOtherInvocations(Method(bdevMock, write));
    Verify(OverloadedMethod(rtreeMock, Get, void(const char *, int32_t, void **,
                                                 size_t *, uint8_t *)))
        .Exactly(1);

    DaqDB::OffloadRqst::removePool.put(poller.requests[0]);
    delete[] poller.requests;
}
