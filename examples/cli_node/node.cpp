/**
 * Copyright 2017-2018 Intel Corporation.
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

#include <iostream>

#include <asio/ip/tcp.hpp>
#include <asio/signal_set.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/bind.hpp>
#include <boost/format.hpp>
#include <boost/program_options.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

#include <boost/log/attributes/named_scope.hpp>
#include <boost/log/attributes/timer.hpp>
#include <boost/log/common.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sources/logger.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/console.hpp>

#include <atomic>

#include <daqdb/KVStoreBase.h>

#include "nodeCli.h"

using namespace std;
using boost::format;
using namespace boost::algorithm;

namespace po = boost::program_options;

namespace logging = boost::log;
namespace bt = logging::trivial;
namespace attrs = boost::log::attributes;
namespace keywords = boost::log::keywords;
namespace src = boost::log::sources;

typedef char KeyType[16];

BOOST_LOG_INLINE_GLOBAL_LOGGER_DEFAULT(
    lg, src::severity_logger_mt<logging::trivial::severity_level>)

namespace {
const unsigned short dhtBackBonePort = 11000;
}

int main(int argc, const char *argv[]) {
    unsigned short nodeId = 0;
    auto dhtPort = dhtBackBonePort;
    bool interactiveMode = false;
    std::string pmem_path;
    // @TODO jradtke move below's configs to singel one
    std::string spdk_conf;
    std::string zht_conf;
    std::string neighbors_conf;

    size_t pmem_size = 0;
    size_t alloc_size = 0;

    logging::add_console_log(std::clog,
                             keywords::format = "%TimeStamp%: %Message%");
    logging::add_common_attributes();
    logging::core::get()->add_thread_attribute("Scope", attrs::named_scope());
    logging::core::get()->set_filter(logging::trivial::severity >=
                                     logging::trivial::error);

    po::options_description argumentsDescription{"Options"};
    argumentsDescription.add_options()("help,h", "Print help messages")(
        "dht,d", po::value<unsigned short>(&dhtPort), "DHT Communication port")(
        "nodeid,n", po::value<unsigned short>(&nodeId)->default_value(0),
        "Node ID used to match database file")(
        "interactive,i", "Enable interactive mode")("log,l", "Enable logging")(
        "pmem-path",
        po::value<std::string>(&pmem_path)->default_value("/mnt/pmem/pool.pm"),
        "Rtree persistent memory pool file")(
        "pmem-size",
        po::value<size_t>(&pmem_size)->default_value(8ull * 1024 * 1024 * 1024),
        "Rtree persistent memory pool size")(
        "alloc-unit-size", po::value<size_t>(&alloc_size)->default_value(8),
        "Allocation unit size")(
        "spdk-conf-file,c",
        po::value<std::string>(&spdk_conf)->default_value("../config.spdk"),
        "SPDK configuration file")(
        "zht-conf-file,c",
        po::value<std::string>(&zht_conf)->default_value("../zht.conf"),
        "ZHT configuration file")(
        "neighbors-file,c",
        po::value<std::string>(&neighbors_conf)->default_value("../neighbors.conf"),
        "Neighbors configuration file");

    po::variables_map parsedArguments;
    try {
        po::store(po::parse_command_line(argc, argv, argumentsDescription),
                  parsedArguments);

        if (parsedArguments.count("help")) {
            std::cout << argumentsDescription << endl;
            return 0;
        }
        if (parsedArguments.count("interactive")) {
            interactiveMode = true;
        }
        if (parsedArguments.count("log")) {
            logging::core::get()->set_filter(logging::trivial::severity >=
                                             logging::trivial::debug);
        }

        po::notify(parsedArguments);
    } catch (po::error &parserError) {
        cerr << "Invalid arguments: " << parserError.what() << endl << endl;
        cerr << argumentsDescription << endl;
        return -1;
    }

    DaqDB::Options options;

    std::atomic<int> isRunning; // used to catch SIGTERM, SIGINT
    isRunning = 1;

    options.Runtime.logFunc = [](std::string msg) {
        BOOST_LOG_SEV(lg::get(), bt::debug) << msg << flush;
    };
    options.Runtime.shutdownFunc = [&isRunning]() { isRunning = 0; };
    options.Runtime.spdkConfigFile = spdk_conf;

    options.Dht.Id = nodeId;
    options.Dht.Port = dhtPort;
    options.Dht.configFile = zht_conf;
    options.Dht.neighborsFile = neighbors_conf;
    options.PMEM.poolPath = pmem_path;
    options.PMEM.totalSize = pmem_size;
    options.PMEM.allocUnitSize = alloc_size;
    options.Key.field(0, sizeof(KeyType));

    shared_ptr<DaqDB::KVStoreBase> spKVStore;
    try {
        spKVStore =
            shared_ptr<DaqDB::KVStoreBase>(DaqDB::KVStoreBase::Open(options));
    } catch (DaqDB::OperationFailedException &e) {
        cerr << "Failed to create KVStore: " << e.what() << endl;
        return -1;
    }

    BOOST_LOG_SEV(lg::get(), bt::info)
        << format("DHT node (id=%1%) is running on localhost:%2%") %
               spKVStore->getProperty("daqdb.dht.id") %
               spKVStore->getProperty("daqdb.dht.port")
        << flush;

    if (interactiveMode) {
        DaqDB::nodeCli nodeCli(spKVStore);
        while (nodeCli()) {
            if (!isRunning)
                break;
        }
    } else {
        while (isRunning) {
            sleep(1);
        }
    }

    BOOST_LOG_SEV(lg::get(), bt::info)
        << format("Closing DHT node (id=%1%) on localhost:%2%") %
               spKVStore->getProperty("daqdb.dht.id") %
               spKVStore->getProperty("daqdb.dht.port")
        << flush;

    return 0;
}
