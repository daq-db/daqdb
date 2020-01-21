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

#pragma once

#include <cstddef>
#include <cstdint>

#include "ClassAlloc.h"
#include "GeneralPool.h"

namespace DaqDB {

class SpdkIoBuf {
  public:
    SpdkIoBuf() = default;
    virtual ~SpdkIoBuf() = default;

    virtual SpdkIoBuf *getWriteBuf() = 0;
    virtual void putWriteBuf(SpdkIoBuf *ioBuf) = 0;
    virtual SpdkIoBuf *getReadBuf() = 0;
    virtual void putReadBuf(SpdkIoBuf *ioBuf) = 0;
    virtual void *getSpdkDmaBuf() = 0;
    virtual void setSpdkDmaBuf(void *spdkBuf) = 0;
    virtual int getIdx() = 0;
    virtual void setIdx(int idx) = 0;
};

template <uint32_t Size> class SpdkIoSizedBuf : public SpdkIoBuf {
    friend class SpdkIoBufMgr;

  public:
    SpdkIoSizedBuf(uint32_t _bufSize = 0, int _backIdx = 0);
    virtual ~SpdkIoSizedBuf();

    virtual SpdkIoBuf *getWriteBuf();
    virtual void putWriteBuf(SpdkIoBuf *ioBuf);
    virtual SpdkIoBuf *getReadBuf();
    virtual void putReadBuf(SpdkIoBuf *ioBuf);
    virtual void *getSpdkDmaBuf();
    virtual void setSpdkDmaBuf(void *spdkBuf);
    virtual int getIdx();
    virtual void setIdx(int idx);

  protected:
    static const uint32_t queueDepth = 16;
    void *spdkDmaBuf;
    uint32_t bufSize;
    int backIdx;

    static MemMgmt::GeneralPool<SpdkIoSizedBuf,
                                MemMgmt::ClassAlloc<SpdkIoSizedBuf>>
        writePool;
    static MemMgmt::GeneralPool<SpdkIoSizedBuf,
                                MemMgmt::ClassAlloc<SpdkIoSizedBuf>>
        readPool;
};

template <uint32_t Size>
SpdkIoSizedBuf<Size>::SpdkIoSizedBuf(uint32_t _bufSize, int _backIdx)
    : spdkDmaBuf(0), bufSize(_bufSize), backIdx(_backIdx) {}

template <uint32_t Size> SpdkIoSizedBuf<Size>::~SpdkIoSizedBuf() {
    if (getSpdkDmaBuf())
        spdk_dma_free(getSpdkDmaBuf());
}

template <uint32_t Size> SpdkIoBuf *SpdkIoSizedBuf<Size>::getWriteBuf() {
    return writePool.get();
}

template <uint32_t Size>
void SpdkIoSizedBuf<Size>::putWriteBuf(SpdkIoBuf *ioBuf) {
    writePool.put(dynamic_cast<SpdkIoSizedBuf<Size> *>(ioBuf));
}

template <uint32_t Size> SpdkIoBuf *SpdkIoSizedBuf<Size>::getReadBuf() {
    return readPool.get();
}

template <uint32_t Size>
void SpdkIoSizedBuf<Size>::putReadBuf(SpdkIoBuf *ioBuf) {
    readPool.put(dynamic_cast<SpdkIoSizedBuf<Size> *>(ioBuf));
}

template <uint32_t Size> void *SpdkIoSizedBuf<Size>::getSpdkDmaBuf() {
    return spdkDmaBuf;
}

template <uint32_t Size>
void SpdkIoSizedBuf<Size>::setSpdkDmaBuf(void *spdkBuf) {
    spdkDmaBuf = spdkBuf;
}

template <uint32_t Size> int SpdkIoSizedBuf<Size>::getIdx() { return backIdx; }

template <uint32_t Size> void SpdkIoSizedBuf<Size>::setIdx(int idx) {
    backIdx = idx;
}

template <uint32_t Size>
MemMgmt::GeneralPool<SpdkIoSizedBuf<Size>,
                     MemMgmt::ClassAlloc<SpdkIoSizedBuf<Size>>>
    SpdkIoSizedBuf<Size>::writePool(queueDepth, "writeSpdkIoBufPool");

template <uint32_t Size>
MemMgmt::GeneralPool<SpdkIoSizedBuf<Size>,
                     MemMgmt::ClassAlloc<SpdkIoSizedBuf<Size>>>
    SpdkIoSizedBuf<Size>::readPool(queueDepth, "readSpdkIoBufPool");

class SpdkIoBufMgr {
  public:
    SpdkIoBufMgr();
    virtual ~SpdkIoBufMgr();

    SpdkIoBuf *getIoWriteBuf(uint32_t ioSize, uint32_t align);
    void putIoWriteBuf(SpdkIoBuf *ioBuf);
    SpdkIoBuf *getIoReadBuf(uint32_t ioSize, uint32_t align);
    void putIoReadBuf(SpdkIoBuf *ioBuf);

    static const uint32_t blockSize = 8;
    SpdkIoBuf *block[blockSize];
};

} // namespace DaqDB
