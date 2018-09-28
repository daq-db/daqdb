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

#include <boost/program_options.hpp>
#include <iostream>
#include <memory>

#include "MinidaqAroNode.h"
#include "MinidaqFfNode.h"
#include "MinidaqRoNode.h"
#include "daqdb/KVStoreBase.h"

using namespace std;

namespace po = boost::program_options;

#define MINIDAQ_DEFAULT_FRAGMENT_SIZE 1024
#define MINIDAQ_DEFAULT_PMEM_SIZE 2ull * 1024 * 1024 * 1024
#define MINIDAQ_DEFAULT_PMEM_PATH "/mnt/pmem/fogkv_minidaq.pm"
#define MINIDAQ_DEFAULT_SPDK_CONF "../config.spdk"
#define MINIDAQ_DEFAULT_T_RAMP_MS 500
#define MINIDAQ_DEFAULT_T_TEST_MS 5000
#define MINIDAQ_DEFAULT_T_ITER_MS 10
#define MINIDAQ_DEFAULT_N_THREADS_RO 0
#define MINIDAQ_DEFAULT_N_THREADS_ARO 0
#define MINIDAQ_DEFAULT_N_THREADS_FF 0
#define MINIDAQ_DEFAULT_N_THREADS_EB 0
#define MINIDAQ_DEFAULT_N_SUBDETECTORS 1
#define MINIDAQ_DEFAULT_N_POOLERS 1
#define MINIDAQ_DEFAULT_START_SUB_ID 100
#define MINIDAQ_DEFAULT_PARALLEL true
#define MINIDAQ_DEFAULT_ACCEPT_LEVEL 0.5
#define MINIDAQ_DEFAULT_DELAY 0
#define MINIDAQ_DEFAULT_BASE_CORE_ID 10
#define MINIDAQ_DEFAULT_N_CORES 10
#define MINIDAQ_DEFAULT_LOG false

#define US_IN_MS 1000

std::string results_prefix;
std::string results_all;
std::string tname;
std::string pmem_path;
std::string tid_file;
size_t pmem_size;
std::string spdk_conf;
int nPoolers;
int tIter_ms;
int tTest_ms;
int tRamp_ms;
int delay;
int nCores;
int bCoreId;
size_t fSize;
bool enableLog = MINIDAQ_DEFAULT_LOG;

static void logStd(std::string m) {
    m.append("\n");
    std::cout << m;
}

/** @todo move to MinidaqFogServer for distributed version */
static DaqDB::KVStoreBase *openKVS() {
    DaqDB::Options options;
    options.PMEM.poolPath = pmem_path;
    options.PMEM.totalSize = pmem_size;
    options.PMEM.allocUnitSize = fSize;
    options.Key.field(0, sizeof(DaqDB::MinidaqKey::eventId), true);
    options.Key.field(1, sizeof(DaqDB::MinidaqKey::subdetectorId));
    options.Key.field(2, sizeof(DaqDB::MinidaqKey::runId));
    options.Runtime.numOfPoolers = nPoolers;
    options.Runtime.spdkConfigFile = spdk_conf;
    if (enableLog) {
        options.Runtime.logFunc = logStd;
    }

    return DaqDB::KVStoreBase::Open(options);
}

static uint64_t calcIterations() {
    /** @todo pmem size limitation? */
    return FOGKV_MAX_PRIMARY_ID;
}

static void
runBenchmark(std::vector<std::unique_ptr<DaqDB::MinidaqNode>> &nodes) {
    int nCoresUsed = 0;

    // Configure
    std::cout << "### Configuring..." << endl;
    for (auto &n : nodes) {
        n->SetTimeTest(tTest_ms);
        n->SetTimeRamp(tRamp_ms);
        n->SetTimeIter(tIter_ms * US_IN_MS);
        n->SetDelay(delay);
        n->SetTidFile(tid_file);
        n->SetBaseCoreId(bCoreId + nCoresUsed);
        n->SetMaxIterations(calcIterations());
        nCoresUsed += n->GetThreads();
        n->SetCores(n->GetThreads());
        if (nCoresUsed > nCores) {
            std::cout << "Not enough CPU cores." << endl;
            exit(1);
        }
    }

    // Run
    std::cout << "### Benchmarking..." << endl;
    for (auto &n : nodes) {
        n->Run();
    }

    // Wait for results
    for (auto &n : nodes) {
        n->Wait();
    }
    std::cout << "### Done." << endl;

    // Show results
    std::cout << "### Results:" << endl;
    for (auto &n : nodes) {
        n->Show();
        if (!results_prefix.empty()) {
            n->Save(results_prefix);
        }
        if (!results_all.empty()) {
            n->SaveSummary(results_all, tname);
        }
    }
}

int main(int argc, const char *argv[]) {
    bool isParallel = MINIDAQ_DEFAULT_PARALLEL;
    double acceptLevel = 0;
    int startSubId = 0;
    int nAroTh = 0;
    int nRoTh = 0;
    int nFfTh = 0;
    int nEbTh = 0;
    int subId = 0;
    int nSub = 0;

    po::options_description genericOpts("Generic options");
    genericOpts.add_options()("help,h", "Print help messages")(
        "time-test",
        po::value<int>(&tTest_ms)->default_value(MINIDAQ_DEFAULT_T_TEST_MS),
        "Desired test duration in milliseconds.")(
        "time-iter",
        po::value<int>(&tIter_ms)->default_value(MINIDAQ_DEFAULT_T_ITER_MS),
        "Desired iteration duration in milliseconds (defines measurement time "
        "for OPS estimation of a single histogram sample.")(
        "time-ramp",
        po::value<int>(&tRamp_ms)->default_value(MINIDAQ_DEFAULT_T_RAMP_MS),
        "Desired ramp up time in milliseconds.")(
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
        "test-name", po::value<std::string>(&tname), "Test name.")(
        "n-poolers",
        po::value<int>(&nPoolers)->default_value(MINIDAQ_DEFAULT_N_POOLERS),
        "Total number of FogKV pooler threads.")(
        "base-core-id",
        po::value<int>(&bCoreId)->default_value(MINIDAQ_DEFAULT_BASE_CORE_ID),
        "Base core ID for minidaq worker threads.")(
        "n-cores",
        po::value<int>(&nCores)->default_value(MINIDAQ_DEFAULT_N_CORES),
        "Number of cores for minidaq worker threads.")(
        "start-delay",
        po::value<int>(&delay)->default_value(MINIDAQ_DEFAULT_DELAY),
        "Delays start of the test on each worker thread.")(
        "tid-file", po::value<std::string>(&tid_file),
        "If set a file with thread IDs of benchmark worker threads will be "
        "generated.")("log,l", "Enable logging")(
        "spdk-conf-file,c", po::value<std::string>(&spdk_conf)
                                ->default_value(MINIDAQ_DEFAULT_SPDK_CONF),
        "SPDK configuration file");

    po::options_description readoutOpts("Readout-specific options");
    readoutOpts.add_options()("n-ro", po::value<int>(&nRoTh)->default_value(
                                          MINIDAQ_DEFAULT_N_THREADS_RO),
                              "Number of readout threads.")(
        "n-aro",
        po::value<int>(&nAroTh)->default_value(MINIDAQ_DEFAULT_N_THREADS_ARO),
        "Number of asynchronous readout threads.")(
        "sub-id",
        po::value<int>(&subId)->default_value(MINIDAQ_DEFAULT_START_SUB_ID),
        "Unique subdetector ID.")(
        "fragment-size",
        po::value<size_t>(&fSize)->default_value(MINIDAQ_DEFAULT_FRAGMENT_SIZE),
        "Single fragment size in bytes.");

    po::options_description filteringOpts("Filtering-specific options");
    filteringOpts.add_options()("n-eb", po::value<int>(&nEbTh)->default_value(
                                            MINIDAQ_DEFAULT_N_THREADS_EB),
                                "Number of event building threads.")(
        "n-ff",
        po::value<int>(&nFfTh)->default_value(MINIDAQ_DEFAULT_N_THREADS_FF),
        "Number of fast filtering threads.")(
        "start-sub-id", po::value<int>(&startSubId)
                            ->default_value(MINIDAQ_DEFAULT_START_SUB_ID),
        "Start subdetector ID.")("n-sub", po::value<int>(&nSub)->default_value(
                                              MINIDAQ_DEFAULT_N_SUBDETECTORS),
                                 "Total number of subdetectors.")(
        "serial,s",
        "If set, readout and collector threads will run in parellel. "
        "Otherwise, collector nodes will wait until readout threads complete.")(
        "acceptance", po::value<double>(&acceptLevel)
                          ->default_value(MINIDAQ_DEFAULT_ACCEPT_LEVEL),
        "Event acceptance level.");

    po::options_description argumentsDescription;
    argumentsDescription.add(genericOpts).add(readoutOpts).add(filteringOpts);
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

    if (parsedArguments.count("log")) {
        enableLog = true;
    }
    if (parsedArguments.count("serial")) {
        isParallel = false;
    }

    if (nEbTh) {
        cerr << "Event builders not supported" << endl;
        return -1;
    }

    if (!nRoTh && !nAroTh && !nFfTh && !nEbTh) {
        std::cout << "Nothing to do. Specify at least one worker thread."
                  << endl;
        return 0;
    }

    try {
        std::cout << "### Opening FogKV..." << endl;
        auto kvs = openKVS();
        std::cout << "### Done." << endl;
        std::vector<std::unique_ptr<DaqDB::MinidaqNode>>
            nodes; // Configure nodes
        if (nRoTh) {
            std::cout << "### Configuring readout nodes..." << endl;
            if (nAroTh) {
                std::cout << "Cannot mix readout modes." << endl;
                return 0;
            }
            unique_ptr<DaqDB::MinidaqRoNode> nodeRo(
                new DaqDB::MinidaqRoNode(kvs));
            nodeRo->SetSubdetectorId(subId);
            nodeRo->SetThreads(nRoTh);
            nodeRo->SetFragmentSize(fSize);
            nodes.push_back(std::move(nodeRo));
            std::cout << "### Done." << endl;
        }

        if (nAroTh) {
            std::cout << "### Configuring asynchronous readout nodes..."
                      << endl;
            if (nRoTh) {
                std::cout << "Cannot mix readout modes." << endl;
                return 0;
            }
            unique_ptr<DaqDB::MinidaqAroNode> nodeAro(
                new DaqDB::MinidaqAroNode(kvs));
            nodeAro->SetSubdetectorId(subId);
            nodeAro->SetThreads(nAroTh);
            nodeAro->SetFragmentSize(fSize);
            nodes.push_back(std::move(nodeAro));
            std::cout << "### Done." << endl;
        }

        // Run readout nodes only, if requested
        if (!isParallel) {
            runBenchmark(nodes);
            nodes.clear();
        }

        if (nFfTh) {
            std::cout << "### Configuring fast-filtering nodes..." << endl;
            unique_ptr<DaqDB::MinidaqFfNode> nodeFf(
                new DaqDB::MinidaqFfNode(kvs));
            nodeFf->SetBaseSubdetectorId(startSubId);
            nodeFf->SetSubdetectors(nSub);
            nodeFf->SetThreads(nFfTh);
            nodeFf->SetAcceptLevel(acceptLevel);
            nodes.push_back(std::move(nodeFf));
            std::cout << "### Done." << endl;
        }

        // Run remaining nodes
        runBenchmark(nodes);
    } catch (std::exception &e) {
        std::cerr << e.what() << std::endl;
        return 0;
    }

    return 0;
}
