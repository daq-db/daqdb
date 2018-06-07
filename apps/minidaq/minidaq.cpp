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

#include <boost/program_options.hpp>
#include <iostream>
#include <memory>

#include "FogKV/KVStoreBase.h"
#include "MinidaqAsyncReadoutNode.h"
#include "MinidaqReadoutNode.h"

using namespace std;

namespace po = boost::program_options;

#define MINIDAQ_DEFAULT_FRAGMENT_SIZE 10240
#define MINIDAQ_DEFAULT_PMEM_SIZE 512 * 1024 * 1024
#define MINIDAQ_DEFAULT_PMEM_PATH "/mnt/pmem/pmemkv.dat"
#define MINIDAQ_DEFAULT_T_RAMP_S 2
#define MINIDAQ_DEFAULT_T_TEST_S 10
#define MINIDAQ_DEFAULT_T_ITER_US 100
#define MINIDAQ_DEFAULT_N_THREADS_RO 1
#define MINIDAQ_DEFAULT_N_THREADS_ARO 0
#define MINIDAQ_DEFAULT_N_THREADS_FF 0
#define MINIDAQ_DEFAULT_N_THREADS_EB 0

/** @todo move to MinidaqFogServer for distributed version */
static FogKV::KVStoreBase *openKVS(std::string &pmemPath, size_t pmemSize) {
    // todo remove redundant stuff once initialization is fixed
    FogKV::Options options;
    options.PMEM.Path = pmemPath;
    options.PMEM.Size = pmemSize;
    options.Key.field(0, sizeof(FogKV::MinidaqKey::eventId), true);
    options.Key.field(1, sizeof(FogKV::MinidaqKey::subdetectorId));
    options.Key.field(2, sizeof(FogKV::MinidaqKey::runId));

    return FogKV::KVStoreBase::Open(options);
}

int main(int argc, const char *argv[]) {
    std::string results_prefix;
    std::string results_all;
    FogKV::KVStoreBase *kvs;
    std::string pmem_path;
    std::string tname;
    size_t pmem_size;
    size_t fSize;
    int tIter_us;
    int tTest_s;
    int tRamp_s;
    int nRaTh;
    int nFfTh;
    int nEbTh;
    int nRTh;

    po::options_description argumentsDescription{"Options"};
    argumentsDescription.add_options()("help,h", "Print help messages")(
        "readout",
        po::value<int>(&nRTh)->default_value(MINIDAQ_DEFAULT_N_THREADS_RO),
        "Number of readout threads. If no modes are specified, this is the "
        "default with one thread.")(
        "readout-async",
        po::value<int>(&nRaTh)->default_value(MINIDAQ_DEFAULT_N_THREADS_ARO),
        "Number of asynchronous readout threads.")(
        "fast-filtering",
        po::value<int>(&nFfTh)->default_value(MINIDAQ_DEFAULT_N_THREADS_FF),
        "Number of fast filtering threads.")(
        "event-building",
        po::value<int>(&nEbTh)->default_value(MINIDAQ_DEFAULT_N_THREADS_EB),
        "Number of event building threads.")(
        "fragment-size",
        po::value<size_t>(&fSize)->default_value(MINIDAQ_DEFAULT_FRAGMENT_SIZE),
        "Fragment size in bytes in case of readout mode.")(
        "time-test",
        po::value<int>(&tTest_s)->default_value(MINIDAQ_DEFAULT_T_TEST_S),
        "Desired test duration in seconds.")(
        "time-iter",
        po::value<int>(&tIter_us)->default_value(MINIDAQ_DEFAULT_T_ITER_US),
        "Desired iteration duration in microseconds (defines measurement time "
        "for OPS estimation of a single histogram sample.")(
        "time-ramp",
        po::value<int>(&tRamp_s)->default_value(MINIDAQ_DEFAULT_T_RAMP_S),
        "Desired ramp up time in seconds.")(
        "pmem-path", po::value<std::string>(&pmem_path)
                         ->default_value(MINIDAQ_DEFAULT_PMEM_PATH),
        "Persistent memory pool file.")(
        "pmem-size",
        po::value<size_t>(&pmem_size)->default_value(MINIDAQ_DEFAULT_PMEM_SIZE),
        "Persistent memory pool size.")(
        "out-prefix", po::value<std::string>(&results_prefix),
        "If set CSV result files will be generated with the desired prefix and "
        "path.")("out-summary", po::value<std::string>(&results_all),
                 "If set CSV line will be appended to the specified file.")(
        "test-name", po::value<std::string>(&tname), "Test name.");

    po::variables_map parsedArguments;
    try {
        po::store(po::parse_command_line(argc, argv, argumentsDescription),
                  parsedArguments);

        if (parsedArguments.count("help")) {
            std::cout << argumentsDescription << endl;
            return 0;
        }

        po::notify(parsedArguments);
    } catch (po::error &parserError) {
        cerr << "Invalid arguments: " << parserError.what() << endl << endl;
        cerr << argumentsDescription << endl;
        return -1;
    }

    if (nFfTh || nEbTh) {
        cerr << "Data collectors not supported" << endl;
        return -1;
    }

    try {
        kvs = openKVS(pmem_path, pmem_size);
        std::vector<std::unique_ptr<FogKV::MinidaqNode>>
            nodes; // Configure nodes
        if (nRTh) {
            unique_ptr<FogKV::MinidaqReadoutNode> nodeReadout(
                new FogKV::MinidaqReadoutNode(kvs));
            nodeReadout->SetThreads(nRTh);
            nodeReadout->SetFragmentSize(fSize);
            nodes.push_back(std::move(nodeReadout));
        }

        if (nRaTh) {
            unique_ptr<FogKV::MinidaqAsyncReadoutNode> nodeAsyncReadout(
                new FogKV::MinidaqAsyncReadoutNode(kvs));
            nodeAsyncReadout->SetThreads(nRaTh);
            nodeAsyncReadout->SetFragmentSize(fSize);
            nodes.push_back(std::move(nodeAsyncReadout));
        }

        for (auto &n : nodes) {
            n->SetTimeTest(tTest_s);
            n->SetTimeRamp(tRamp_s);
            n->SetTimeIter(tIter_us);
        }

        // Run
        for (auto &n : nodes) {
            n->Run();
        }

        // Wait for results
        for (auto &n : nodes) {
            n->Wait();
        }

        // Show results
        for (auto &n : nodes) {
            n->Show();
            if (!results_prefix.empty()) {
                n->Save(results_prefix);
            }
            if (!results_all.empty()) {
                n->SaveSummary(results_all, tname);
            }
        }
    } catch (std::exception &e) {
        std::cerr << e.what() << std::endl;
        return 0;
    }

    return 0;
}
