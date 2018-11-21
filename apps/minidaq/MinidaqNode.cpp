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

#include <atomic>
#include <fstream>
#include <future>
#include <iomanip>
#include <iostream>
#include <sys/syscall.h>
#include <thread>
#include <unistd.h>
#include <vector>

#include "MinidaqNode.h"
#include "MinidaqTimerHR.h"

namespace DaqDB {

MinidaqNode::MinidaqNode(KVStoreBase *kvs)
    : _kvs(kvs), _stopped(false), _statsReady(false)
#ifdef WITH_INTEGRITY_CHECK
      ,
      _nIntegrityChecks(0), _nIntegrityErrors(0)
#endif /* WITH_INTEGRITY_CHECK */
{
}

MinidaqNode::~MinidaqNode() {}

void MinidaqNode::SetTimeTest(int ms) { _tTest_ms = ms; }

void MinidaqNode::SetTimeRamp(int ms) { _tRamp_ms = ms; }

void MinidaqNode::SetTimeIter(int us) { _tIter_us = us; }

void MinidaqNode::SetThreads(int n) { _nTh = n; }

void MinidaqNode::SetDelay(int s) { _delay_s = s; }

void MinidaqNode::SetTidFile(std::string &tidFile) { _tidFile = tidFile; }

void MinidaqNode::SetCores(int n) { _nCores = n; }

void MinidaqNode::SetBaseCoreId(int id) { _baseCoreId = id; }

void MinidaqNode::SetMaxIterations(uint64_t n) { _maxIterations = n; }

void MinidaqNode::SetStopOnError(bool stop) { _stopOnError = stop; }

void MinidaqNode::SetLive(bool live) { _live = live; }

int MinidaqNode::GetThreads() { return _nTh; }

void MinidaqNode::_Affinity(int executorId) {
    int cid = _baseCoreId + (executorId % _nCores);

    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cid, &cpuset);
    pthread_setaffinity_np(pthread_self(), sizeof(cpuset), &cpuset);

    std::stringstream msg;
    int tid = syscall(__NR_gettid);
    msg << "Executor " << executorId << ": " << _GetType()
        << ", thread id: " << tid << ", core id: " << cid << std::endl;
    std::cout << msg.str();
    if (!_tidFile.empty()) {
        msg.str("");
        msg << tid << std::endl;
        std::ofstream ofs;
        ofs.open(_tidFile, std::ios_base::app);
        ofs << msg.str();
        ofs.close();
    }
}

MinidaqStats MinidaqNode::_Execute(int executorId) {
    std::atomic<std::uint64_t> c_err;
    std::atomic<std::uint64_t> c;
    MinidaqTimerHR timerSample;
    MinidaqTimerHR timerTest;
    MinidaqStats stats;
    MinidaqSample s;
    uint64_t i = 0;

    // Pre-test
    _Affinity(executorId);
    if (_delay_s) {
        std::this_thread::sleep_for(std::chrono::seconds(_delay_s));
    }

    // Prepare reusable key
    MinidaqKey minidaqKey;
    _Setup(executorId, minidaqKey);

    // Ramp up
    timerTest.Restart_ms(_tRamp_ms);
    while (!timerTest.IsExpired() && !_stopped) {
        i++;
        if (_maxIterations && i > _maxIterations) {
            _stopped = true;
            break;
        }
        s.nRequests++;
        try {
            _Task(minidaqKey, c, c_err);
            _NextKey(minidaqKey);
        } catch (...) {
        }
    }

    // Record samples
    timerTest.Restart_ms(_tTest_ms);
    c = 0;
    c_err = 0;
    while (!timerTest.IsExpired()) {
        // Timer precision per iteration
        auto avg_r = (s.nRequests + 10) / 10;
        s.Reset();
        timerSample.Restart_us(_tIter_us);
        do {
            i++;
            if (_maxIterations && i > _maxIterations) {
                _stopped = true;
                break;
            }
            s.nRequests++;
            try {
                _Task(minidaqKey, c, c_err);
            } catch (...) {
                s.nErrRequests++;
                if (_stopOnError)
                    _stopped = true;
            }
            _NextKey(minidaqKey);
        } while (!_stopped &&
                 ((s.nRequests % avg_r) || !timerSample.IsExpired()));
        if (_stopped)
            break;
        s.interval_ns = timerSample.GetElapsed_ns();
        s.nCompletions = c.fetch_and(0ULL);
        s.nErrCompletions = c_err.fetch_and(0ULL);
        if (_live)
            stats.ShowSample(_GetType() + "_" + std::to_string(executorId), s);
        stats.RecordSample(s);
    }

    _stopped = true;

    // Wait for all completions
    do {
        c = 0;
        c_err = 0;
        std::this_thread::sleep_for(std::chrono::nanoseconds(s.interval_ns));
    } while (c || c_err);

    std::stringstream msg;
    msg << "Total number of task iterations: " << std::to_string(i - 1)
        << std::endl;
    std::cout << msg.str();

    return stats;
}

void MinidaqNode::Run() {
    int i;

    // Clear file with thread IDs
    if (!_tidFile.empty()) {
        std::ofstream ofs;
        ofs.open(_tidFile, std::ofstream::out | std::ofstream::trunc);
        ofs.close();
    }

    for (i = 0; i < _nTh; i++) {
        _futureVec.emplace_back(
            std::async(std::launch::async, &MinidaqNode::_Execute, this, i));
    }
}

void MinidaqNode::Wait() {
    for (const auto &f : _futureVec) {
        f.wait();
    }

    for (auto &f : _futureVec) {
        auto s = f.get();
        _statsVec.push_back(s);
        _statsAll.Combine(s);
    }
    _statsReady = true;

    std::cout << "Done!" << std::endl;
}

void MinidaqNode::Show() {
    int i = 0;

    if (!_statsReady) {
        return;
    }

    _statsAll.DumpSummaryHeader();

    for (auto &s : _statsVec) {
        std::cout << "th-" << i++ << "-" << _GetType() << std::endl;
        s.DumpSummary();
    }

    std::cout << "all-" << _GetType() << std::endl;
    _statsAll.DumpSummary();

#ifdef WITH_INTEGRITY_CHECK
    std::cout << "Integrity checks: " << std::to_string(_nIntegrityChecks)
              << std::endl;
    std::cout << "Integrity errors: " << std::to_string(_nIntegrityErrors)
              << std::endl;
#endif /* WITH_INTEGRITY_CHECK */
}

void MinidaqNode::Save(std::string &fp) {
    int i = 0;

    if (!_statsReady || fp.empty()) {
        return;
    }

    for (auto &s : _statsVec) {
        std::stringstream ss;
        ss << fp << "-thread-" << i++ << "-" << _GetType();
        s.Dump(ss.str());
    }
    std::stringstream ss;
    ss << fp << "-thread-all-" << _GetType();
    _statsAll.Dump(ss.str());
}

void MinidaqNode::SaveSummary(std::string &fs, std::string &tname) {
    if (!_statsReady || fs.empty()) {
        return;
    }

    _statsAll.DumpSummary(fs, tname);
}

#ifdef WITH_INTEGRITY_CHECK
char MinidaqNode::_GetBufferByte(MinidaqKey &key, size_t i) {
    return ((key.eventId + i) % 256);
}

void MinidaqNode::_FillBuffer(MinidaqKey &key, char *buf, size_t s) {
    for (int i = 0; i < s; i++) {
        buf[i] = _GetBufferByte(key, i);
    }
}

void MinidaqNode::_CheckBuffer(MinidaqKey &key, char *buf, size_t s) {
    std::stringstream msg;
    unsigned char b_exp;
    unsigned char b_act;
    bool err = false;

    _nIntegrityChecks++;
    for (int i = 0; i < s; i++) {
        b_exp = _GetBufferByte(key, i);
        b_act = buf[i];
        if (b_exp != b_act) {
            if (!err) {
                err = true;
                msg << "Integrity check failed (" << _GetType()
                    << ") EventId=" << key.eventId
                    << " SubdetectorId=" << key.subdetectorId << std::endl;
                _nIntegrityErrors++;
            }
            msg << "  buf[" << i << "] = "
                << "0x" << std::hex << static_cast<int>(b_act) << " Expected: "
                << "0x" << static_cast<int>(b_exp) << std::dec << std::endl;
        }
    }
    if (err)
        std::cout << msg.str();
}
#endif /* WITH_INTEGRITY_CHECK */
}
