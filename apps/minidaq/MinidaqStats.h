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
    MinidaqStats(const std::vector<MinidaqStats> &rVector);
    ~MinidaqStats();
    MinidaqStats(const MinidaqStats &other);
    MinidaqStats operator=(MinidaqStats &&other);

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
