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

#include <iostream>

#include <asio/ip/tcp.hpp>
#include <asio/signal_set.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/bind.hpp>
#include <boost/program_options.hpp>

#include <atomic>

#include <daqdb/KVStoreBase.h>

#include "config.h"
#include "debug.h"
#include <uc.h>

#define RUN_USE_CASE(name)                                                     \
    if (name) {                                                                \
        BOOST_LOG_SEV(lg::get(), bt::info) << "Test completed successfully";   \
    } else {                                                                   \
        BOOST_LOG_SEV(lg::get(), bt::info) << "Test failed";                   \
    }

using namespace std;

using namespace boost::algorithm;

namespace po = boost::program_options;

typedef char KeyType[16];

const string zhtConf = "zht-ft.conf";
const string neighborConf = "neighbor-ft.conf";

typedef char DEFAULT_KeyType[16];

int main(int argc, const char *argv[]) {
    string configFile;

    logging::add_console_log(std::clog,
                             keywords::format = "%TimeStamp%: %Message%");
    logging::add_common_attributes();
    logging::core::get()->add_thread_attribute("Scope", attrs::named_scope());
    logging::core::get()->set_filter(logging::trivial::severity >=
                                     logging::trivial::error);
    logging::core::get()->set_filter(logging::trivial::severity >=
                                     logging::trivial::debug);

    po::options_description argumentsDescription{"Options"};
    argumentsDescription.add_options()("help,h", "Print help messages")(
        "config-file,c",
        po::value<string>(&configFile)->default_value("functest.cfg"),
        "Configuration file");

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

    DaqDB::Options options;
    options.Runtime.logFunc = [](std::string msg) {
        BOOST_LOG_SEV(lg::get(), bt::debug) << msg << flush;
    };
    initKvsOptions(options, configFile);

    /* ZHT configuration files must be prepared before KVStore is created */
    prepare_zht_tests(zhtConf, neighborConf);

    shared_ptr<DaqDB::KVStoreBase> spKVStore;
    try {
        spKVStore =
            shared_ptr<DaqDB::KVStoreBase>(DaqDB::KVStoreBase::Open(options));
    } catch (DaqDB::OperationFailedException &e) {
        cerr << "Failed to create KVStore: " << e.what() << endl;
        return -1;
    }

    BOOST_LOG_SEV(lg::get(), bt::info)
        << format("DHT node (id=%1%) is running on %2%:%3%") %
               spKVStore->getProperty("daqdb.dht.id") %
               spKVStore->getProperty("daqdb.listener.ip") %
               spKVStore->getProperty("daqdb.listener.port")
        << flush;

    RUN_USE_CASE(use_case_sync_base(spKVStore));
    RUN_USE_CASE(use_case_async_base(spKVStore));
    RUN_USE_CASE(use_case_sync_offload(spKVStore));
    RUN_USE_CASE(use_case_async_offload(spKVStore));
    RUN_USE_CASE(use_case_zht_connect(spKVStore, zhtConf, neighborConf));

    BOOST_LOG_SEV(lg::get(), bt::info) << "Closing DHT node." << flush;

    cleanup_zht_tests(zhtConf, neighborConf);

    return 0;
}
