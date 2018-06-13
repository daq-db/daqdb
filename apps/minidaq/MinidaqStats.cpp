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

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iostream>
#include <numeric>
#include <stdio.h>
#include <system_error>

#include "MinidaqStats.h"

#define NSECS_IN_SEC 1000000000ULL
#define OPS_MIN 1
#define OPS_MAX 100000000ULL
#define OPS_IN_MOPS 1000000.0
#define SHOW_VALUE(x)                                                          \
    std::cout << "|" << std::right << std::setw(10) << std::setprecision(6)    \
              << x << "|"
#define SHOW_HEADER(x)                                                         \
    std::cout << "|" << std::left << std::setw(10) << x << "|"
#define SHOW_LINE_SEP(x)                                                       \
    for (int i = 0; i < x; i++) {                                              \
        std::cout << "|----------|";                                           \
    }

using namespace std;

namespace FogKV {

MinidaqStats::MinidaqStats() {
    int err;

    err = hdr_init(OPS_MIN, OPS_MAX, 5, &_histogramRps);
    if (err) {
        throw std::system_error(err, std::system_category());
    }
    err = hdr_init(OPS_MIN, OPS_MAX, 5, &_histogramCps);
    if (err) {
        throw std::system_error(err, std::system_category());
    }
    err = hdr_init(OPS_MIN, OPS_MAX, 5, &_histogramRpsErr);
    if (err) {
        throw std::system_error(err, std::system_category());
    }
    err = hdr_init(OPS_MIN, OPS_MAX, 5, &_histogramCpsErr);
    if (err) {
        throw std::system_error(err, std::system_category());
    }
}

MinidaqStats::MinidaqStats(const std::vector<MinidaqStats> &rVector)
    : MinidaqStats() {
    for (const auto &r : rVector) {
        Combine(r);
    }
}

MinidaqStats::~MinidaqStats() {
    if (_histogramRps) {
        hdr_close(_histogramRps);
    }
    if (_histogramCps) {
        hdr_close(_histogramCps);
    }
    if (_histogramRpsErr) {
        hdr_close(_histogramRpsErr);
    }
    if (_histogramCpsErr) {
        hdr_close(_histogramCpsErr);
    }
}

MinidaqStats::MinidaqStats(const MinidaqStats &other) : MinidaqStats() {
    hdr_add(_histogramRps, other._histogramRps);
    hdr_add(_histogramCps, other._histogramCps);
    hdr_add(_histogramRpsErr, other._histogramRpsErr);
    hdr_add(_histogramCpsErr, other._histogramCpsErr);
    _nIterations = other._nIterations;
    _totalTime_ns = other._totalTime_ns;
}

MinidaqStats MinidaqStats::operator=(MinidaqStats &&other) {
    _histogramRps = other._histogramRps;
    _histogramCps = other._histogramCps;
    _histogramRpsErr = other._histogramRpsErr;
    _histogramCpsErr = other._histogramCpsErr;
    _nIterations = other._nIterations;
    _totalTime_ns = other._totalTime_ns;

    other._histogramRps = nullptr;
    other._histogramCps = nullptr;
    other._histogramRpsErr = nullptr;
    other._histogramCpsErr = nullptr;
}

void MinidaqStats::Combine(const MinidaqStats &stats) {
    if (!stats._nIterations) {
        return;
    }
    _nIterations += stats._nIterations;
    _totalTime_ns += stats._totalTime_ns;
    _nOverflows += hdr_add(_histogramRps, stats._histogramRps);
    _nOverflows += hdr_add(_histogramCps, stats._histogramCps);
    _nOverflows += hdr_add(_histogramRpsErr, stats._histogramRpsErr);
    _nOverflows += hdr_add(_histogramCpsErr, stats._histogramCpsErr);
    _nOverflows += stats._nOverflows;
}

void MinidaqStats::RecordSample(const MinidaqSample &s) {
    bool ok;

    if (!s.interval_ns) {
        return;
    }

    if (s.nRequests) {
        ok = hdr_record_value(_histogramRps,
                              (s.nRequests * NSECS_IN_SEC) / s.interval_ns);
        if (!ok) {
            _nOverflows++;
        }
    }
    if (s.nCompletions) {
        ok = hdr_record_value(_histogramCps,
                              (s.nCompletions * NSECS_IN_SEC) / s.interval_ns);
        if (!ok) {
            _nOverflows++;
        }
    }
    if (s.nErrRequests) {
        ok = hdr_record_value(_histogramRpsErr,
                              (s.nErrRequests * NSECS_IN_SEC) / s.interval_ns);
        if (!ok) {
            _nOverflows++;
        }
    }
    if (s.nErrCompletions) {
        ok = hdr_record_value(_histogramCpsErr,
                              (s.nErrCompletions * NSECS_IN_SEC) /
                                  s.interval_ns);
        if (!ok) {
            _nOverflows++;
        }
    }
    _nIterations++;
    _totalTime_ns += s.interval_ns;
}

void MinidaqStats::Dump() {
    double avg;

    std::cout << "Iterations: " << _nIterations << std::endl;
    std::cout << "Histogram overflows: " << _nOverflows << std::endl;

    if (!_nIterations) {
        return;
    }

    std::cout << "Average iteration time: "
              << 1000.0 * double(_totalTime_ns) / double(_nIterations) << " us"
              << std::endl;

    std::cout << "########################################################"
              << std::endl;
    std::cout << "   Requests per Second [MOPS]" << std::endl;
    std::cout << "########################################################"
              << std::endl;
    hdr_percentiles_print(_histogramRps, stdout, 1, 1000000.0, CLASSIC);
    std::cout << "########################################################"
              << std::endl;
    std::cout << "   Completions per Second [MOPS]" << std::endl;
    std::cout << "########################################################"
              << std::endl;
    hdr_percentiles_print(_histogramCps, stdout, 1, 1000000.0, CLASSIC);
    std::cout << "########################################################"
              << std::endl;
    std::cout << "   Error Requests per Second [MOPS]" << std::endl;
    std::cout << "########################################################"
              << std::endl;
    hdr_percentiles_print(_histogramRpsErr, stdout, 1, 1000000.0, CLASSIC);
    std::cout << "########################################################"
              << std::endl;
    std::cout << "   Error Completions per Second [MOPS]" << std::endl;
    std::cout << "########################################################"
              << std::endl;
    hdr_percentiles_print(_histogramCpsErr, stdout, 1, 1000000.0, CLASSIC);
}

void MinidaqStats::_DumpCsv(const std::string &fp,
                            enum MinidaqMetricType histo_type) {
    struct hdr_histogram *h;
    std::string h_name;
    FILE *pFile;

    switch (histo_type) {
    case MINIDAQ_METRIC_RPS:
        h = _histogramRps;
        h_name = "histo-mrps";
        break;
    case MINIDAQ_METRIC_RPS_ERR:
        h = _histogramRpsErr;
        h_name = "histo-mrps-err";
        break;
    case MINIDAQ_METRIC_CPS:
        h = _histogramRps;
        h_name = "histo-mcps";
        break;
    case MINIDAQ_METRIC_CPS_ERR:
        h = _histogramCpsErr;
        h_name = "histo-mcps-err";
        break;
    default:
        return;
    }

    std::string fn = fp + "-" + h_name + ".csv";
    pFile = fopen(fn.c_str(), "w");
    if (pFile == NULL) {
        std::cout << "Cannot open file: " << fn << std::endl;
    }
    hdr_percentiles_print(h, pFile, 1, 1000000.0, CSV);
    fclose(pFile);
}

void MinidaqStats::Dump(const std::string &fp) {
    _DumpCsv(fp, MINIDAQ_METRIC_RPS);
    _DumpCsv(fp, MINIDAQ_METRIC_RPS_ERR);
    _DumpCsv(fp, MINIDAQ_METRIC_CPS);
    _DumpCsv(fp, MINIDAQ_METRIC_CPS_ERR);
}

void MinidaqStats::_DumpSummary(enum MinidaqMetricType histo_type) {
    struct hdr_histogram *h;
    std::string h_name;

    switch (histo_type) {
    case MINIDAQ_METRIC_RPS:
        h = _histogramRps;
        h_name = "mrps";
        break;
    case MINIDAQ_METRIC_RPS_ERR:
        h = _histogramRpsErr;
        h_name = "mrps-err";
        break;
    case MINIDAQ_METRIC_CPS:
        h = _histogramCps;
        h_name = "mcps";
        break;
    case MINIDAQ_METRIC_CPS_ERR:
        h = _histogramCpsErr;
        h_name = "mcps-err";
        break;
    default:
        return;
    }

    cout.setf(ios::floatfield, ios::fixed);
    SHOW_HEADER(h_name);
    SHOW_VALUE((h->total_count ? hdr_mean(h) / OPS_IN_MOPS : 0));
    SHOW_VALUE((h->total_count ? hdr_stddev(h) / OPS_IN_MOPS : 0));
    SHOW_VALUE((h->total_count ? hdr_min(h) / OPS_IN_MOPS : 0));
    SHOW_VALUE((h->total_count ? hdr_max(h) / OPS_IN_MOPS : 0));
    SHOW_VALUE((h->total_count));
    std::cout << std::endl;
}

void MinidaqStats::_DumpSummaryCsv(std::fstream &f,
                                   enum MinidaqMetricType histo_type) {
    struct hdr_histogram *h;

    switch (histo_type) {
    case MINIDAQ_METRIC_RPS:
        h = _histogramRps;
        break;
    case MINIDAQ_METRIC_RPS_ERR:
        h = _histogramRpsErr;
        break;
    case MINIDAQ_METRIC_CPS:
        h = _histogramRps;
        break;
    case MINIDAQ_METRIC_CPS_ERR:
        h = _histogramCpsErr;
        break;
    default:
        return;
    }

    f << (h->total_count ? hdr_mean(h) / OPS_IN_MOPS : 0) << ","
      << (h->total_count ? hdr_stddev(h) / OPS_IN_MOPS : 0) << ","
      << (h->total_count ? hdr_min(h) / OPS_IN_MOPS : 0) << ","
      << (h->total_count ? hdr_max(h) / OPS_IN_MOPS : 0);
}

void MinidaqStats::DumpSummary() {
    std::stringstream ss;

    _DumpSummary(MINIDAQ_METRIC_RPS);
    _DumpSummary(MINIDAQ_METRIC_CPS);
    _DumpSummary(MINIDAQ_METRIC_RPS_ERR);
    _DumpSummary(MINIDAQ_METRIC_CPS_ERR);
    SHOW_LINE_SEP(6);
    std::cout << std::endl;
}

void MinidaqStats::DumpSummaryHeader() {
    std::cout << "Iterations: " << _nIterations << std::endl;
    std::cout << "Overflows: " << _nOverflows << std::endl;

    std::vector<std::string> f;

    SHOW_HEADER("Metric");
    SHOW_HEADER("Mean");
    SHOW_HEADER("StdDev");
    SHOW_HEADER("Min");
    SHOW_HEADER("Max");
    SHOW_HEADER("TotalCnt");
    std::cout << std::endl;
    SHOW_LINE_SEP(6);
    std::cout << std::endl;
}

void MinidaqStats::DumpSummary(const std::string &file,
                               const std::string &name) {
    fstream f;

    f.open(file, ios_base::out | ios_base::in);
    if (f.is_open()) {
        f.close();
        f.open(file, ios_base::app);
    } else {
        f.clear();
        f.open(file, ios_base::out);
        f << "TestName,"
          << "mrps-Mean,mrps-StdDev,mrps-Min,mrps-Max,"
          << "mcps-Mean,mcps-StdDev,mcps-Min,mcps-Max,"
          << "mrps-e-Mean,mrps-e-StdDev,mrps-e-Min,mrps-e-Max,"
          << "mcps-e-Mean,mcps-e-StdDev,mcps-e-Min,mcps-e-Max" << std::endl;
    }
    f << name << ",";
    _DumpSummaryCsv(f, MINIDAQ_METRIC_RPS);
    f << ",";
    _DumpSummaryCsv(f, MINIDAQ_METRIC_CPS);
    f << ",";
    _DumpSummaryCsv(f, MINIDAQ_METRIC_RPS_ERR);
    f << ",";
    _DumpSummaryCsv(f, MINIDAQ_METRIC_CPS_ERR);
    f << std::endl;

    f.close();
}
}
