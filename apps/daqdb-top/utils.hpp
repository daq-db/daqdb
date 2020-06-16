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
#include <daqdb/KVStoreBase.h>
#include <boost/program_options.hpp>
#include <daqdb/Options.h>
#include <libpmem.h>
#include <config/KVStoreConfig.h>


struct __attribute__((packed)) CliNodeKey {
    uint8_t eventId[5];
    uint8_t detectorId;
    uint16_t componentId;
};

DaqDB::Key assign_key(DaqDB::KVStoreBase *m_kvs, uint64_t &eventId, int detectorId ) {
    DaqDB::Key key = m_kvs->AllocKey(DaqDB::KeyValAttribute::NOT_BUFFERED);

    CliNodeKey *daqdb_key = reinterpret_cast<CliNodeKey *>(key.data());
    daqdb_key->detectorId = 0;
    daqdb_key->componentId = detectorId;
    memcpy(&daqdb_key->eventId, &eventId, sizeof(daqdb_key->eventId));
    return key;
}



// Setting CPU affinity when executing the benchmark (DHT Threads and DHT Poolers)
void SetAffinity(int baseCore) {
    int cid = baseCore ;
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cid, &cpuset);
    int rc = pthread_setaffinity_np(pthread_self(), sizeof(cpuset), &cpuset);
    if (rc != 0) {
       std::cerr << "Error calling pthread_setaffinity_np: " << rc << "\n";
    }
}


bool thread_wait(int ms, std::shared_future<double> f ){
   std::future_status status;
     if (!ms) {
         f.wait();
     } else {
         status = f.wait_for(std::chrono::milliseconds(ms));
         if (status != std::future_status::ready) {
             return false;
         }
     }
   return true;
}

