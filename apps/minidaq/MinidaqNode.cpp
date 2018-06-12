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

#include <atomic>
#include <future>
#include <iostream>
#include <thread>
#include <vector>

#include "MinidaqNode.h"
#include "MinidaqTimerHR.h"

#include <iomanip>

namespace FogKV {

MinidaqNode::MinidaqNode(KVStoreBase *kvs)
    : _kvs(kvs), _stopped(false), _statsReady(false) {}

MinidaqNode::~MinidaqNode() {}

void MinidaqNode::SetTimeTest(int s) { _tTest_s = s; }

void MinidaqNode::SetTimeRamp(int s) { _tRamp_s = s; }

void MinidaqNode::SetTimeIter(int us) { _tIter_us = us; }

void MinidaqNode::SetBaseSubdetectorId(int id) { _baseSubdetectorId = id; }

void MinidaqNode::SetSubdetectors(int n) { _nSubdetectors = n; }

void MinidaqNode::_SetThreads(int n) { _nTh = n; }

MinidaqStats MinidaqNode::_Execute(int executorId) {
    std::atomic<std::uint64_t> c_err;
    uint64_t event_id = executorId;
    std::atomic<std::uint64_t> c;
    MinidaqTimerHR timerSample;
    MinidaqTimerHR timerTest;
    MinidaqStats stats;
    MinidaqSample s;
    uint64_t avg_r;

    // Ramp up
    timerTest.Restart_s(_tRamp_s);
    while (!timerTest.IsExpired()) {
        s.nRequests++;
        event_id += _nTh;
        _Task(event_id, executorId, c, c_err);
    }

    // Record samples
    timerTest.Restart_s(_tTest_s);
    while (!timerTest.IsExpired() && !_stopped) {
        // Timer precision per iteration
        avg_r = (s.nRequests + 10) / 10;
        s.Reset();
        c = 0;
        c_err = 0;
        timerSample.Restart_us(_tIter_us);
        do {
            event_id += _nTh;
            try {
                _Task(event_id, executorId, c, c_err);
                s.nRequests++;
            } catch (...) {
                s.nErrRequests++;
            }
        } while (!_stopped &&
                 ((s.nRequests % avg_r) || !timerSample.IsExpired()));
        s.nCompletions = c;
        s.nErrCompletions = c_err;
        s.interval_ns = timerSample.GetElapsed_ns();
        stats.RecordSample(s);
    }

    _stopped = true;

    // Wait for all completions
    do {
        c = 0;
        c_err = 0;
        std::this_thread::sleep_for(
            std::chrono::nanoseconds(10 * s.interval_ns));
    } while (c || c_err);

    return stats;
}

void MinidaqNode::Run() {
    int i;

    _Setup();

    for (i = 0; i < _nTh; i++) {
        _futureVec.emplace_back(
            std::async(std::launch::async, &MinidaqNode::_Execute, this, i));
    }
}

void MinidaqNode::Wait() {
    int i;

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
}
