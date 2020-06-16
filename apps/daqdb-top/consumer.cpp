// DAQDB-top consumer application
// USAGE:  see README file
// 
// Author: Adam Abed Abud
// Mail: adam.abed.abud@cern.ch
// Last update: June 16, 2020


// Standard 
#include <iostream>
#include <boost/filesystem.hpp>
#include <string>     
#include <chrono>
#include <sys/syscall.h>
#include <thread>
#include <unistd.h>
#include <future>

// DAQDB
#include "utils.hpp"
#include <daqdb/KVStoreBase.h>
#include <boost/program_options.hpp>
#include <daqdb/Options.h>
#include <libpmem.h>
#include <config/KVStoreConfig.h>


namespace po = boost::program_options;

#define DEFAULT_FRAGMENT_SIZE 1024
#define DEFAULT_NUMBER_EVENTS 1000000
#define DEFAULT_PMEM_POOL_PATH "/mnt/pmem/fogkv_minidaq.pm"
#define DEFAULT_SPDK_CONF "../config.spdk"
#define DEFAULT_PMEM_POOL_SIZE 10ull * 1024 * 1024 * 1024 // 10 GiB
#define DEFAULT_PMEM_ALLOC_UNIT_SIZE 1024
#define DEFAULT_T_RAMP_MS 500
#define DEFAULT_T_TEST_MS 5000
#define DEFAULT_T_ITER_MS 10
#define DEFAULT_N_THREADS_RO 1
#define DEFAULT_N_THREADS_ARO 0
#define DEFAULT_N_THREADS_FF 0
#define DEFAULT_N_THREADS_EB 0
#define DEFAULT_N_THREADS_DHT 1
#define DEFAULT_B_ID_DHT 0
#define DEFAULT_N_SUBDETECTORS 1
#define DEFAULT_N_POOLERS 0
#define DEFAULT_START_SUB_ID 100
#define DEFAULT_PARALLEL true
#define DEFAULT_SERVER false
#define DEFAULT_ACCEPT_LEVEL 0.0
#define DEFAULT_DELAY 0
#define DEFAULT_BASE_CORE_ID 20
#define DEFAULT_N_CORES 29
#define DEFAULT_FILTERING_CORE 26
#define DEFAULT_LOG false
#define DEFAULT_COLLECTOR_DELAY_US 100
#define DEFAULT_ITERATIONS 0
#define DEFAULT_STOPONERROR true
#define DEFAULT_LIVE false
#define DEFAULT_MAX_READY_KEYS 0
#define DEFAULT_SATELLITE false
#define DEFAULT_CONF "client.cfg"
#define DEFAULT_FR_DISTRO "const"
#define DEFAULT_OFFLOAD_ALLOC_UNIT_SIZE 1024





// Set CPU affinity for consumer thread
void SetAffinityFiltering(int executorId) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(executorId, &cpuset);
    int rc = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
    if (rc != 0) {
       std::cerr << "Error calling pthread_setaffinity_np Readout: " << rc << "\n";
    }
}




void initKvsOptions(DaqDB::Options &options, const std::string &configFile) {

    /* Set default values */
    options.dht.id = 0;
  

    options.key.field(0, sizeof(CliNodeKey::eventId), true);
    options.key.field(1, sizeof(CliNodeKey::detectorId));
    options.key.field(2, sizeof(CliNodeKey::componentId));

    options.offload.allocUnitSize = DEFAULT_OFFLOAD_ALLOC_UNIT_SIZE;
    if (boost::filesystem::exists(configFile)) {
       std::cout << "### Reading configuration file: " << configFile << "\n";
       std::stringstream errorMsg;
       if (!DaqDB::readConfiguration(configFile, options, errorMsg)) {
           std::cout << "### Failed to read minidaq configuration file.\n"
                     << errorMsg.str() << "\n";
       }
       std::cout << "### Done parsing configuration file.\n";
    }
}


DaqDB::Value daqdb_get(DaqDB::KVStoreBase *kvs, DaqDB::Key &key) {
    try {
        auto result = kvs->Get(key);
        return result;
    } catch (DaqDB::OperationFailedException &e) {
        if (e.status()() == DaqDB::KEY_NOT_FOUND) {
            std::cout<< "Key not found"<< key.data();

        } else {
            std::cout<< "Error: cannot get element: " << e.status().to_string()
                     << std::flush;
        }
    }
    return DaqDB::Value();
}



double reader_thread(int starting_events, int number_events, std::shared_ptr<DaqDB::KVStoreBase> m_kvs, 
          int componentId, int coreId) {

   // Set affinity of the consumer thread
   SetAffinityFiltering(coreId);

   // Benchmark execution time of the consumer thread
   auto start = std::chrono::high_resolution_clock::now();
   for (uint64_t i=starting_events; i<number_events; i++) {
       DaqDB::Key daqdb_key = assign_key(m_kvs.get(), i, componentId);
       DaqDB::Value daqdb_val;
       try {
           daqdb_val = daqdb_get(m_kvs.get(), daqdb_key);
           //std::cout << "daqdb_val: " << daqdb_val.data() << "\n" ; 
       } catch (...) {
           daqdb_val = daqdb_get(m_kvs.get(), daqdb_key);
           std::cout << "Object not found. Check key.\n"; 
           throw;
       }
   }
   auto end = std::chrono::high_resolution_clock::now();
   double thread_execution_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();   
   return thread_execution_time ;
}



int main(int argc, const char *argv[]) {
    std::cout << "===================================\n";
    std::cout << "       Started DAQDB CONSUMER      \n"; 
    std::cout << "===================================\n";

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
    int FilteringCores ; 
    int nCoresUsed = 0;
    int bCoreId;
    int nDhtThreads;
    int bDhtId ;
    int maxReadyKeys;
    int number_events ; 
    size_t fSize = DEFAULT_FRAGMENT_SIZE ;
    bool enableLog = DEFAULT_LOG;
    size_t maxIters;
    bool stopOnError = DEFAULT_STOPONERROR;
    bool live = DEFAULT_LIVE;
    bool satellite = DEFAULT_SATELLITE;
    std::string frDistro = DEFAULT_FR_DISTRO;
    std::string configFile;
    bool singleNode = false;
    std::unique_ptr<DaqDB::DhtNeighbor> localDht;
    bool isParallel = DEFAULT_PARALLEL;
    bool isServer = DEFAULT_SERVER;
    double acceptLevel = 0;
    int collectorDelay = 0;
    int startSubId = 0;
    int nAroTh = 0;
    int nRoTh = 0;
    int nFfTh = 1; // Setting at least 1 filtering node to avoid floating point exception
    int nEbTh = 0;
    int subId = 0;
    int nSub = 0;

    po::options_description genericOpts("Generic options");
    genericOpts.add_options()("help,h", "Print help messages")("number-events", po::value<int>(&number_events)->default_value(DEFAULT_NUMBER_EVENTS), "Desired number of events to process." )(
        "time-test",
        po::value<int>(&tTest_ms)->default_value(DEFAULT_T_TEST_MS),
        "Desired test duration in milliseconds.")(
        "time-iter",
        po::value<int>(&tIter_ms)->default_value(DEFAULT_T_ITER_MS),
        "Desired iteration duration in milliseconds (defines measurement time "
        "for OPS estimation of a single histogram sample.")(
        "time-ramp",
        po::value<int>(&tRamp_ms)->default_value(DEFAULT_T_RAMP_MS),
        "Desired ramp up time in milliseconds.")(
        "max-iters",
        po::value<size_t>(&maxIters)->default_value(DEFAULT_ITERATIONS),
        "In non-zero, defines the maximum number of iterations per thread.")(
        "stopOnError", "If set, test will not continue after first error")(
        "live", "If set, live results will be displayed")(
        "satellite", "If set, local storage backend will be disabled")(
        "pmem-path", po::value<std::string>(&pmem_path)
                         ->default_value(DEFAULT_PMEM_POOL_PATH),
        "Persistent memory pool file.")(
        "pmem-size",
        po::value<size_t>(&pmem_size)->default_value(DEFAULT_PMEM_POOL_SIZE),
        "Persistent memory pool size.")(
        "serverMode",
        "If set, minidaq will open KVS and wait for external requests. ")(
        "out-prefix", po::value<std::string>(&results_prefix),
        "If set CSV result files will be generated with the desired prefix and "
        "path.")("out-summary", po::value<std::string>(&results_all),
                 "If set CSV line will be appended to the specified file.")(
        "test-name", po::value<std::string>(&tname), "Test name.")(
        "n-poolers",
        po::value<int>(&nPoolers)->default_value(DEFAULT_N_POOLERS),
        "Total number of DaqDB pooler threads.")(
        "n-dht-threads", po::value<int>(&nDhtThreads)
                             ->default_value(DEFAULT_N_THREADS_DHT),
        "Total number of DaqDB DHT threads.")(
        "base-dht-id", po::value<int>(&bDhtId)
                             ->default_value(DEFAULT_B_ID_DHT),
        "Base DHT ID to be used by DAQDB clients.")(
        "base-core-id",
        po::value<int>(&bCoreId)->default_value(DEFAULT_BASE_CORE_ID),
        "Base core ID for DAQDB and minidaq worker threads.")(
        "n-cores",
        po::value<int>(&nCores)->default_value(DEFAULT_N_CORES),
        "Total number of cores available for DAQDB and minidaq worker "
        "threads.")(
        "filtering-cores",
        po::value<int>(&FilteringCores)->default_value(DEFAULT_FILTERING_CORE),
        "Set the core on which the filtering application will "
        "run.")("max-ready-keys",
                    po::value<int>(&maxReadyKeys)
                        ->default_value(DEFAULT_MAX_READY_KEYS),
                    "Size of the queue holding keys ready for collecting.")(
        "start-delay",
        po::value<int>(&delay)->default_value(DEFAULT_DELAY),
        "Delays start of the test on each worker thread.")(
        "tid-file", po::value<std::string>(&tid_file),
        "If set a file with thread IDs of benchmark worker threads will be "
        "generated.")("log,l", "Enable logging")(
        "config-file,c",
        po::value<std::string>(&configFile)->default_value(DEFAULT_CONF),
        "Configuration file defining DHT nodes.")(
        "spdk-conf-file,c", po::value<std::string>(&spdk_conf)
                                ->default_value(DEFAULT_SPDK_CONF),
        "SPDK configuration file");


    po::options_description readoutOpts("Readout-specific options");
    readoutOpts.add_options()("n-ro", po::value<int>(&nRoTh)->default_value(
                                          DEFAULT_N_THREADS_RO),
                              "Number of readout threads.")(
        "n-aro",
        po::value<int>(&nAroTh)->default_value(DEFAULT_N_THREADS_ARO),
        "Number of asynchronous readout threads.")(
        "sub-id",
        po::value<int>(&subId)->default_value(DEFAULT_START_SUB_ID),
        "Unique subdetector ID.")(
        "fragment-size",
        po::value<size_t>(&fSize)->default_value(DEFAULT_FRAGMENT_SIZE),
        "Base fragment size in bytes.")(
        "fragment-distro", po::value<std::string>(&frDistro)
                               ->default_value(DEFAULT_FR_DISTRO),
        "Distribution for fragment size, supported values: "
        "const (1, default), poisson (lambda = fragment size)");

    po::options_description filteringOpts("Filtering-specific options");
    filteringOpts.add_options()("n-eb", po::value<int>(&nEbTh)->default_value(
                                            DEFAULT_N_THREADS_EB),
                                "Number of event building threads.")(
        "n-ff",
        po::value<int>(&nFfTh)->default_value(DEFAULT_N_THREADS_FF),
        "Number of fast filtering threads.")(
        "start-sub-id", po::value<int>(&startSubId)
                            ->default_value(DEFAULT_START_SUB_ID),
        "Start subdetector ID.")("n-sub", po::value<int>(&nSub)->default_value(
                                              DEFAULT_N_SUBDETECTORS),
                                 "Total number of subdetectors.")(
        "serial,s",
        "If set, readout and collector threads will run in parellel. "
        "Otherwise, collector nodes will wait until readout threads complete.")(
        "acceptance", po::value<double>(&acceptLevel)
                          ->default_value(DEFAULT_ACCEPT_LEVEL),
        "Event acceptance level.")(
        "delay", po::value<int>(&collectorDelay)
                     ->default_value(DEFAULT_COLLECTOR_DELAY_US),
        "If set, collector threads will wait delay us between requests for "
        "event, if not yet available.");

    po::options_description argumentsDescription;
    argumentsDescription.add(genericOpts).add(readoutOpts).add(filteringOpts);
    po::variables_map parsedArguments;

    try {
        po::store(po::parse_command_line(argc, argv, argumentsDescription),
                  parsedArguments);

        if (parsedArguments.count("help")) {
            std::cout << argumentsDescription << std::endl;
            return 0;
        }

        po::notify(parsedArguments);
    }
    catch (po::error &parserError) {
        std::cerr << "Invalid arguments: " << parserError.what() << std::endl << std::endl;
        std::cerr << argumentsDescription << std::endl;
        return -1;
    }


    DaqDB::Options options;
    options.runtime.baseCoreId = bCoreId;
    options.runtime.numOfPollers = nPoolers;
    nCoresUsed += nPoolers;
    options.dht.numOfDhtThreads = nDhtThreads;
    //options.dht.baseDhtId = bDhtId ;
    options.runtime.maxReadyKeys = maxReadyKeys;
    // The following three lines are needed by the consumer application 
    //options.pmem.poolPath = pmem_path;
    //options.pmem.totalSize = pmem_size;
    //options.pmem.allocUnitSize = DEFAULT_PMEM_ALLOC_UNIT_SIZE;


    if (!satellite) {
        std::cout << "### Satellite mode disabled\n";
        nCoresUsed += nDhtThreads;
    }
    if (nCoresUsed > nCores) {
        std::cout << "### Not enough CPU cores.\n";
        exit(1);
    }


    initKvsOptions(options, configFile);


    // Create KVS
    std::shared_ptr<DaqDB::KVStoreBase> m_kvs;
    try {
        m_kvs =
            std::shared_ptr<DaqDB::KVStoreBase>(DaqDB::KVStoreBase::Open(options));

    } catch (DaqDB::OperationFailedException &e) {
        std::cout << "Failed to create KVStore: " << e.what() << "\n";
        return -1;
    }

    std::cout<<"### Connected to KVS successfully\n";

    // Set the structure of the key
    uint64_t eventId = 1;
    uint64_t componentId = 1;
 
    // Setting CPU affinity of the DHT core      
    std::cout << "Setting CPU affinity to core: " << bCoreId << "\n" ;
    SetAffinity(bCoreId);
 

    // Start benchmarking 
    std::vector<std::shared_future<double>> _futureVec;
    float events_thread = number_events/nFfTh ;    
    
    std::cout<<"### Started benchmark \n"; 
    // Create async threads. Divide number of events by number of threads and assign
    // each portiion to the right thread so that there is no PMEM:ALLOCATION issue.
    for (uint64_t j=0; j< nFfTh ;j++){
            std::cout << "Creating new thread \n" ;
            _futureVec.emplace_back(std::async(std::launch::async, reader_thread, 
                  j*events_thread, j*events_thread+events_thread, 
                  m_kvs, componentId, FilteringCores+j));             
    }


    for (const auto &f : _futureVec) {
      while(!thread_wait(10000, f)) {
           std::cout << "Still waiting for consumer thread to finish! \n" ;
      }
    }


    std::cout<<"### Finished benchmark \n" ;

    std::cout << "================ Statistics ================\n";
    std::cout << "Number of threads:\t\t\t" << nFfTh << "\n" ; 
    std::cout << "Number of events per thread:\t\t\t" << events_thread << "\n";

    for (auto &f: _futureVec) {
      auto s = f.get();
      std::cout << "Execution time [ms]: " << s << "\tRate [kIOPS]: " << events_thread/s << "\n";
  }

    std::cout << "\n============================================\n";


}


