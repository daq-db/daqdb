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

#pragma once

#include <atomic>
#include <future>
#include <string>
#include <vector>

#include "MinidaqStats.h"
#include "daqdb/KVStoreBase.h"

namespace DaqDB {

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
    void SetMaxIterations(uint64_t n);
    void SetStopOnError(bool stop);
    int GetThreads();

  protected:
    virtual void _Task(MinidaqKey &key, std::atomic<std::uint64_t> &cnt,
                       std::atomic<std::uint64_t> &cntErr) = 0;
    virtual void _Setup(int executorId, MinidaqKey &key) = 0;
    virtual void _NextKey(MinidaqKey &key) = 0;
#ifdef WITH_INTEGRITY_CHECK
    char _GetBufferByte(MinidaqKey &key, size_t i);
    void _FillBuffer(MinidaqKey &key, char *buf, size_t s);
    void _CheckBuffer(MinidaqKey &key, char *buf, size_t s);
#endif /* WITH_INTEGRITY_CHECK */
    virtual std::string _GetType() = 0;

    KVStoreBase *_kvs;
    int _runId = 599;
    int _nTh = 1; // number of worker threads
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

    std::vector<std::future<MinidaqStats>> _futureVec;
    std::vector<MinidaqStats> _statsVec;
    MinidaqStats _statsAll;
};
}
