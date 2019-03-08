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

#pragma once

#include <atomic>
#include <future>
#include <string>
#include <vector>

#include "MinidaqStats.h"
#include "daqdb/KVStoreBase.h"

namespace DaqDB {

struct __attribute__((packed)) MinidaqKey {
    uint8_t eventId[5];
    uint8_t detectorId;
    uint16_t componentId;
};

class MinidaqNode {
  public:
    MinidaqNode(KVStoreBase *kvs);
    virtual ~MinidaqNode();

    void Run();
    bool Wait(int ms);
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
    void SetMaxIterations(uint64_t n);
    void SetStopOnError(bool stop);
    void SetLive(bool live);
    void SetLocalOnly(bool local);
    int GetThreads();
    void ShowTreeStats();

  protected:
    virtual void _Task(Key &&key, std::atomic<std::uint64_t> &cnt,
                       std::atomic<std::uint64_t> &cntErr) = 0;
    virtual void _Setup(int executorId) = 0;
    virtual Key _NextKey() = 0;
#ifdef WITH_INTEGRITY_CHECK
    char _GetBufferByte(const Key &key, size_t i);
    void _FillBuffer(const Key &key, char *buf, size_t s);
    bool _CheckBuffer(const Key &key, const char *buf, size_t s);
#endif /* WITH_INTEGRITY_CHECK */
    virtual std::string _GetType() = 0;

    KVStoreBase *_kvs;
    int _nTh = 1;            // number of worker threads
    bool _localOnly = false; // single-node benchmark
#ifdef WITH_INTEGRITY_CHECK
    std::atomic<std::uint64_t> _nIntegrityChecks;
    std::atomic<std::uint64_t> _nIntegrityErrors;
#endif /* WITH_INTEGRITY_CHECK */

  private:
    MinidaqStats _Execute(int executorId);
    void _Affinity(int executorId);

    int _tTest_ms = 0;         // desired test duration in milliseconds
    int _tRamp_ms = 0;         // desired test ramp duration in milliseconds
    int _tIter_us = 0;         // desired iteration time in microseconds
    int _delay_s = 0;          // desired delay before starting benchmark loop
    std::atomic_bool _stopped; // signals first thread stopped execution
    std::atomic_bool _statsReady; // signals all results are available
    std::string _tidFile;         // file to store worker thread IDs
    int _nCores = 1;     // number of cores available for minidaq workers
    int _baseCoreId = 0; // base core id for minidaq workers
    uint64_t _maxIterations = 0; // maximum number of iterations per thread
    bool _stopOnError = false;   // break test on first error
    bool _live = false;          // show live results

    std::vector<std::future<MinidaqStats>> _futureVec;
    std::vector<MinidaqStats> _statsVec;
    MinidaqStats _statsAll;
};
}
