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

#include "hdr_histogram.h"
#include <cstdint>
#include <time.h>
#include <vector>

namespace DaqDB {

enum MinidaqMetricType {
    MINIDAQ_METRIC_RPS,
    MINIDAQ_METRIC_RPS_ERR,
    MINIDAQ_METRIC_CPS,
    MINIDAQ_METRIC_CPS_ERR,
};

struct MinidaqSample {
    MinidaqSample() = default;
    MinidaqSample(uint64_t r, uint64_t c, uint64_t e_r, uint64_t e_c,
                  uint64_t ns)
        : nRequests(r), nCompletions(c), nErrRequests(e_r),
          nErrCompletions(e_c), interval_ns(ns){};
    void Reset() {
        nRequests = 0;
        nCompletions = 0;
        nErrRequests = 0;
        nErrCompletions = 0;
        interval_ns = 0;
    }
    uint64_t nRequests = 0;
    uint64_t nCompletions = 0;
    uint64_t nErrRequests = 0;
    uint64_t nErrCompletions = 0;
    uint64_t interval_ns = 0;
};

class MinidaqStats {
  public:
    MinidaqStats();
    explicit MinidaqStats(const std::vector<MinidaqStats> &rVector);
    ~MinidaqStats();
    MinidaqStats(const MinidaqStats &other);
    MinidaqStats &operator=(MinidaqStats &&other);

    static void ShowSample(const std::string &info, const MinidaqSample &s);
    void RecordSample(const MinidaqSample &s);
    void Combine(const MinidaqStats &stats);

    void Dump();
    void Dump(const std::string &fp);
    void DumpSummary();
    void DumpSummary(const std::string &fp, const std::string &name);
    void DumpSummaryHeader();

  private:
    void _DumpCsv(const std::string &fp, enum MinidaqMetricType histo_type);
    void _DumpSummary(enum MinidaqMetricType histo_type);
    void _DumpSummaryCsv(std::fstream &f, enum MinidaqMetricType histo_type);

    struct hdr_histogram *_histogramRps = nullptr;
    struct hdr_histogram *_histogramCps = nullptr;
    struct hdr_histogram *_histogramRpsErr = nullptr;
    struct hdr_histogram *_histogramCpsErr = nullptr;

    uint64_t _nIterations = 0;
    uint64_t _totalTime_ns = 0;
    uint64_t _nOverflows = 0;
};
}
