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

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <iostream>
#include <memory>

#include "MinidaqAroNode.h"
#include "MinidaqFfNode.h"
#include "MinidaqFfNodeSeq.h"
#include "MinidaqRoNode.h"
#include "config/Configuration.h"
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
#define MINIDAQ_DEFAULT_N_THREADS_DHT 1
#define MINIDAQ_DEFAULT_N_SUBDETECTORS 1
#define MINIDAQ_DEFAULT_N_POOLERS 1
#define MINIDAQ_DEFAULT_START_SUB_ID 100
#define MINIDAQ_DEFAULT_PARALLEL true
#define MINIDAQ_DEFAULT_SERVER false
#define MINIDAQ_DEFAULT_ACCEPT_LEVEL 0.5
#define MINIDAQ_DEFAULT_DELAY 0
#define MINIDAQ_DEFAULT_BASE_CORE_ID 0
#define MINIDAQ_DEFAULT_N_CORES 24
#define MINIDAQ_DEFAULT_LOG false
#define MINIDAQ_DEFAULT_COLLECTOR_DELAY_US 100
#define MINIDAQ_DEFAULT_ITERATIONS 0
#define MINIDAQ_DEFAULT_STOPONERROR false
#define MINIDAQ_DEFAULT_LIVE false
#define MINIDAQ_DEFAULT_MAX_READY_KEYS 4 * 1024 * 1024
#define MINIDAQ_DEFAULT_SATELLITE false
#define MINIDAQ_DEFAULT_CONF "minidaq.cfg"

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
int nCoresUsed = 0;
int bCoreId;
int nDhtThreads;
int maxReadyKeys;
size_t fSize;
bool enableLog = MINIDAQ_DEFAULT_LOG;
size_t maxIters;
bool stopOnError = MINIDAQ_DEFAULT_STOPONERROR;
bool live = MINIDAQ_DEFAULT_LIVE;
bool satellite = MINIDAQ_DEFAULT_SATELLITE;
std::string configFile;
bool singleNode = false;
std::unique_ptr<DaqDB::DhtNeighbor> localDht;

static void logStd(std::string m) {
    m.append("\n");
    std::cout << m;
}

/** @todo move to MinidaqFogServer for distributed version */
static std::unique_ptr<DaqDB::KVStoreBase> openKVS() {
    DaqDB::Options options;

    if (boost::filesystem::exists(configFile)) {
        std::cout << "### Reading minidaq configuration file... " << endl;
        std::stringstream errorMsg;
        if (!DaqDB::readConfiguration(configFile, options, errorMsg)) {
            std::cout << "### Failed to read minidaq configuration file. "
                      << endl
                      << errorMsg.str() << std::endl;
        }
        std::cout << "### Done. " << endl;
    }
    options.pmem.poolPath = pmem_path;
    options.pmem.totalSize = pmem_size;
    options.pmem.allocUnitSize = fSize;
    options.key.field(0, sizeof(DaqDB::MinidaqKey::eventId), true);
    options.key.field(1, sizeof(DaqDB::MinidaqKey::detectorId));
    options.key.field(2, sizeof(DaqDB::MinidaqKey::componentId));
    options.runtime.numOfPollers = nPoolers;
    nCoresUsed += nPoolers;
    options.dht.numOfDhtThreads = nDhtThreads;
    nCoresUsed += nDhtThreads;
    if (nCoresUsed > nCores) {
        std::cout << "Not enough CPU cores." << endl;
        exit(1);
    }
    options.runtime.baseCoreId = bCoreId;
    options.runtime.maxReadyKeys = maxReadyKeys;
    if (satellite) {
        options.mode = DaqDB::OperationalMode::SATELLITE;
    } else {
        options.mode = DaqDB::OperationalMode::STORAGE;
        if (!options.dht.neighbors.size()) {
            localDht.reset(new DaqDB::DhtNeighbor);
            localDht->ip = "localhost";
            localDht->port = 31850;
            localDht->local = true;
            options.dht.neighbors.push_back(localDht.get());
            singleNode = true;
        }
    }
    if (enableLog) {
        options.runtime.logFunc = logStd;
    }

    return std::unique_ptr<DaqDB::KVStoreBase>(
        DaqDB::KVStoreBase::Open(options));
}

static void
runBenchmark(std::vector<std::unique_ptr<DaqDB::MinidaqNode>> &nodes) {
    if (!nodes.size())
        return;

    // Configure
    std::cout << "### Configuring..." << endl;
    for (auto &n : nodes) {
        n->SetTimeTest(tTest_ms);
        n->SetTimeRamp(tRamp_ms);
        n->SetTimeIter(tIter_ms * US_IN_MS);
        n->SetDelay(delay);
        n->SetTidFile(tid_file);
        n->SetBaseCoreId(bCoreId + nCoresUsed);
        n->SetMaxIterations(maxIters);
        n->SetStopOnError(stopOnError);
        n->SetLive(live);
        n->SetLocalOnly(singleNode);
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
    bool showStats = true;
    for (auto &n : nodes) {
        while (!n->Wait(live ? tIter_ms : 0))
            n->ShowTreeStats();
        showStats = false;
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
    bool isServer = MINIDAQ_DEFAULT_SERVER;
    double acceptLevel = 0;
    int collectorDelay = 0;
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
        "max-iters",
        po::value<size_t>(&maxIters)->default_value(MINIDAQ_DEFAULT_ITERATIONS),
        "In non-zero, defines the maximum number of iterations per thread.")(
        "stopOnError", "If set, test will not continue after first error")(
        "live", "If set, live results will be displayed")(
        "satellite", "If set, local storage backend will be disabled")(
        "pmem-path", po::value<std::string>(&pmem_path)
                         ->default_value(MINIDAQ_DEFAULT_PMEM_PATH),
        "Persistent memory pool file.")(
        "pmem-size",
        po::value<size_t>(&pmem_size)->default_value(MINIDAQ_DEFAULT_PMEM_SIZE),
        "Persistent memory pool size.")(
        "serverMode",
        "If set, minidaq will open KVS and wait for external requests. ")(
        "out-prefix", po::value<std::string>(&results_prefix),
        "If set CSV result files will be generated with the desired prefix and "
        "path.")("out-summary", po::value<std::string>(&results_all),
                 "If set CSV line will be appended to the specified file.")(
        "test-name", po::value<std::string>(&tname), "Test name.")(
        "n-poolers",
        po::value<int>(&nPoolers)->default_value(MINIDAQ_DEFAULT_N_POOLERS),
        "Total number of DaqDB pooler threads.")(
        "n-dht-threads", po::value<int>(&nDhtThreads)
                             ->default_value(MINIDAQ_DEFAULT_N_THREADS_DHT),
        "Total number of DaqDB DHT threads.")(
        "base-core-id",
        po::value<int>(&bCoreId)->default_value(MINIDAQ_DEFAULT_BASE_CORE_ID),
        "Base core ID for DAQDB and minidaq worker threads.")(
        "n-cores",
        po::value<int>(&nCores)->default_value(MINIDAQ_DEFAULT_N_CORES),
        "Total number of cores available for DAQDB and minidaq worker "
        "threads.")("max-ready-keys",
                    po::value<int>(&maxReadyKeys)
                        ->default_value(MINIDAQ_DEFAULT_MAX_READY_KEYS),
                    "Size of the queue holding keys ready for collecting.")(
        "start-delay",
        po::value<int>(&delay)->default_value(MINIDAQ_DEFAULT_DELAY),
        "Delays start of the test on each worker thread.")(
        "tid-file", po::value<std::string>(&tid_file),
        "If set a file with thread IDs of benchmark worker threads will be "
        "generated.")("log,l", "Enable logging")(
        "config-file,c",
        po::value<string>(&configFile)->default_value(MINIDAQ_DEFAULT_CONF),
        "Configuration file defining DHT nodes.")(
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
        "Event acceptance level.")(
        "delay", po::value<int>(&collectorDelay)
                     ->default_value(MINIDAQ_DEFAULT_COLLECTOR_DELAY_US),
        "If set, collector threads will wait delay us between requests for "
        "event, if not yet available.");

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
    if (parsedArguments.count("serverMode")) {
        isServer = true;
    }
    if (parsedArguments.count("stopOnError")) {
        stopOnError = true;
    }
    if (parsedArguments.count("live")) {
        live = true;
    }
    if (parsedArguments.count("satellite")) {
        satellite = true;
    }

    if (nEbTh) {
        cerr << "Event builders not supported" << endl;
        return -1;
    }

    try {
        std::cout << "### Opening DaqDB..." << endl;
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
                new DaqDB::MinidaqRoNode(kvs.get()));
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
                new DaqDB::MinidaqAroNode(kvs.get()));
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
            nCoresUsed -= (nRoTh + nAroTh);
        }

        if (nFfTh) {
            std::cout << "### Configuring fast-filtering nodes..." << endl;
            unique_ptr<DaqDB::MinidaqFfNode> nodeFf(
                maxReadyKeys ? (new DaqDB::MinidaqFfNode(kvs.get()))
                             : (new DaqDB::MinidaqFfNodeSeq(kvs.get())));
            nodeFf->SetBaseSubdetectorId(startSubId);
            nodeFf->SetSubdetectors(nSub);
            nodeFf->SetThreads(nFfTh);
            nodeFf->SetAcceptLevel(acceptLevel);
            nodeFf->SetRetryDelay(collectorDelay);
            nodes.push_back(std::move(nodeFf));
            std::cout << "### Done." << endl;
        }

        // Run remaining nodes
        runBenchmark(nodes);

        if (isServer) {
            std::cout << "### minidaq server running... Press any key to exit"
                      << endl;
            std::getchar();
        }

    } catch (std::exception &e) {
        std::cerr << e.what() << std::endl;
        return 0;
    }

    return 0;
}
