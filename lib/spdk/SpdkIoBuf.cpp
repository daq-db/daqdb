/**
 *  Copyright (c) 2020 Intel Corporation
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

#include "spdk/env.h"

#include "SpdkIoBuf.h"

namespace DaqDB {

using namespace MemMgmt;

SpdkIoBufMgr::~SpdkIoBufMgr() {}

SpdkIoBuf *SpdkIoBufMgr::getIoWriteBuf(uint32_t ioSize, uint32_t align) {
    SpdkIoBuf *buf = 0;
    uint32_t bufIdx = ioSize / 4096;
    if (bufIdx < 8) {
        buf = block[bufIdx]->getWriteBuf();

        if (!buf->getSpdkDmaBuf())
            buf->setSpdkDmaBuf(
                spdk_dma_zmalloc(static_cast<size_t>(ioSize), align, NULL));
    } else {
        buf = new SpdkIoSizedBuf<1 << 16>(1 << 16, -1);
        buf->setSpdkDmaBuf(
            spdk_dma_zmalloc(static_cast<size_t>(ioSize), align, NULL));
    }
    return buf;
}

void SpdkIoBufMgr::putIoWriteBuf(SpdkIoBuf *ioBuf) {
    if (ioBuf->getIdx() != -1)
        ioBuf->putWriteBuf(ioBuf);
    else {
        spdk_dma_free(ioBuf->getSpdkDmaBuf());
        delete ioBuf;
    }
}

SpdkIoBuf *SpdkIoBufMgr::getIoReadBuf(uint32_t ioSize, uint32_t align) {
    SpdkIoBuf *buf = 0;
    uint32_t bufIdx = ioSize / 4096;
    if (bufIdx < 8) {
        buf = block[bufIdx]->getReadBuf();

        if (!buf->getSpdkDmaBuf())
            buf->setSpdkDmaBuf(
                spdk_dma_zmalloc(static_cast<size_t>(ioSize), align, NULL));
    } else {
        buf = new SpdkIoSizedBuf<1 << 16>(1 << 16, -1);
        buf->setSpdkDmaBuf(
            spdk_dma_zmalloc(static_cast<size_t>(ioSize), align, NULL));
    }
    return buf;
}

void SpdkIoBufMgr::putIoReadBuf(SpdkIoBuf *ioBuf) {
    if (ioBuf->getIdx() != -1)
        ioBuf->putReadBuf(ioBuf);
    else {
        spdk_dma_free(ioBuf->getSpdkDmaBuf());
        delete ioBuf;
    }
}

SpdkIoBufMgr::SpdkIoBufMgr() {
    block[0] = new SpdkIoSizedBuf<4096>(4096, 0);
    block[1] = new SpdkIoSizedBuf<2 * 4096>(2 * 4096, 1);
    block[2] = new SpdkIoSizedBuf<3 * 4096>(3 * 4096, 2);
    block[3] = new SpdkIoSizedBuf<4 * 4096>(4 * 4096, 3);
    block[4] = new SpdkIoSizedBuf<5 * 4096>(5 * 4096, 4);
    block[5] = new SpdkIoSizedBuf<6 * 4096>(6 * 4096, 5);
    block[6] = new SpdkIoSizedBuf<7 * 4096>(7 * 4096, 6);
    block[7] = new SpdkIoSizedBuf<8 * 4096>(8 * 4096, 7);
}

} // namespace DaqDB
