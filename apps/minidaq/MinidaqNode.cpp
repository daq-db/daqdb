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

void MinidaqNode::SetLocalOnly(bool local) { _localOnly = local; }

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
    _Setup(executorId);
    if (_delay_s) {
        std::this_thread::sleep_for(std::chrono::seconds(_delay_s));
    }

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
            Key key = _NextKey();
            _Task(std::move(key), c, c_err);
        } catch (...) {
            if (_stopOnError)
                _stopped = true;
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
                Key key = _NextKey();
                _Task(std::move(key), c, c_err);
            } catch (...) {
                s.nErrRequests++;
                if (_stopOnError)
                    _stopped = true;
            }
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
char MinidaqNode::_GetBufferByte(const Key &key, size_t i) {
    const MinidaqKey *mKeyPtr =
        reinterpret_cast<const MinidaqKey *>(key.data());
    const char *eventId = reinterpret_cast<const char *>(&mKeyPtr->eventId);
    return *(eventId +
             (i % (sizeof(mKeyPtr->eventId) / sizeof(mKeyPtr->eventId[0]))));
}

void MinidaqNode::_FillBuffer(const Key &key, char *buf, size_t s) {
    for (int i = 0; i < s; i++) {
        buf[i] = _GetBufferByte(key, i);
    }
}

bool MinidaqNode::_CheckBuffer(const Key &key, const char *buf, size_t s) {
    const MinidaqKey *mKeyPtr =
        reinterpret_cast<const MinidaqKey *>(key.data());
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
                    << ") EventId=0x";
                for (int j = (sizeof(mKeyPtr->eventId) /
                              sizeof(mKeyPtr->eventId[0])) -
                             1;
                     j >= 0; j--)
                    msg << std::hex
                        << static_cast<unsigned int>(mKeyPtr->eventId[j]);
                msg << std::dec << " SubdetectorId=" << mKeyPtr->componentId
                    << std::endl;
                _nIntegrityErrors++;
            }
            msg << "  buf[" << i << "] = "
                << "0x" << std::hex << static_cast<int>(b_act) << " Expected: "
                << "0x" << static_cast<int>(b_exp) << std::dec << std::endl;
        }
    }
    if (err) {
        std::cout << msg.str();
        return false;
    }

    return true;
}
#endif /* WITH_INTEGRITY_CHECK */
}
