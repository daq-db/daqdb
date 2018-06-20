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

#pragma once

#include <atomic>
#include <future>
#include <string>
#include <vector>

#include "FogKV/KVStoreBase.h"
#include "MinidaqStats.h"

namespace FogKV {

struct MinidaqKey {
    MinidaqKey() : eventId(0), subdetectorId(0), runId(0){};
    MinidaqKey(uint64_t e, uint16_t s, uint16_t r)
        : eventId(e), subdetectorId(s), runId(r) {}
    uint64_t eventId;
    uint16_t subdetectorId;
    uint16_t runId;
};

class MinidaqNode {
  public:
    MinidaqNode(KVStoreBase *kvs);
    virtual ~MinidaqNode();

    void Run();
    void Wait();
    void Show();
    void Save(std::string &fp);
    void SaveSummary(std::string &fs, std::string &tname);
    void SetTimeTest(int s);
    void SetTimeIter(int us);
    void SetTimeRamp(int s);
    void SetThreads(int n);
    void SetDelay(int s);
    void SetTidFile(std::string &tidFile);
    void SetCores(int n);
    void SetBaseCoreId(int id);
    int GetThreads();

  protected:
    virtual void _Task(MinidaqKey &key, std::atomic<std::uint64_t> &cnt,
                       std::atomic<std::uint64_t> &cntErr) = 0;
    virtual void _Setup(int executorId, MinidaqKey &key) = 0;
    virtual void _NextKey(MinidaqKey &key) = 0;
    virtual std::string _GetType() = 0;

    KVStoreBase *_kvs;
    int _runId = 599;
    int _nTh = 1; // number of worker threads

  private:
    MinidaqStats _Execute(int executorId);
    void _Affinity(int executorId);

    int _tTest_s = 0;          // desired test duration in seconds
    int _tRamp_s = 0;          // desired test ramp duration in seconds
    int _tIter_us = 0;         // desired iteration time in microseconds
    int _delay_s = 0;          // desired delay before starting benchmark loop
    std::atomic_bool _stopped; // signals first thread stopped execution
    std::atomic_bool _statsReady; // signals all results are available
    std::string _tidFile;         // file to store worker thread IDs
    int _nCores = 1;     // number of cores available for minidaq workers
    int _baseCoreId = 0; // base core id for minidaq workers

    std::vector<std::future<MinidaqStats>> _futureVec;
    std::vector<MinidaqStats> _statsVec;
    MinidaqStats _statsAll;
};
}
