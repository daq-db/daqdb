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

#include <iostream>

#include <asio/ip/tcp.hpp>
#include <asio/signal_set.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/bind.hpp>
#include <boost/program_options.hpp>

#include <atomic>

#include <daqdb/KVStoreBase.h>

#include "config.h"
#include "debug.h"
#include "tests/tests.h"

using namespace std;

using namespace boost::algorithm;

namespace po = boost::program_options;

typedef char KeyType[16];

typedef char DEFAULT_KeyType[16];

static void init_logger() {
    logging::add_console_log(std::clog, keywords::format = "%Message%");
    logging::add_common_attributes();
    logging::core::get()->add_thread_attribute("Scope", attrs::named_scope());
    logging::core::get()->set_filter(logging::trivial::severity >=
                                     logging::trivial::info);
}

static bool executeTest(string test, function<bool(DaqDB::KVStoreBase *)> fn,
                        DaqDB::KVStoreBase *kvs) {
    DAQDB_INFO << string(80, '-') << endl << test << endl << string(80, '-');

    if (fn(kvs)) {
        DAQDB_INFO << "Test completed successfully" << endl;
        return true;
    } else {
        DAQDB_INFO << "Test failed" << endl;
        return false;
    }
}

int main(int argc, const char *argv[]) {
    string configFile;

    init_logger();

    po::options_description argumentsDescription{"Options"};
    argumentsDescription.add_options()("help,h", "Print help messages")(
        "log,l", "Enable logging")(
        "config-file,c",
        po::value<string>(&configFile)->default_value("functests.cfg"),
        "Configuration file");

    po::variables_map parsedArguments;
    try {
        po::store(po::parse_command_line(argc, argv, argumentsDescription),
                  parsedArguments);

        if (parsedArguments.count("help")) {
            std::cout << argumentsDescription << endl;
            return 0;
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

    DAQDB_INFO << "Functional tests for DAQDB library" << flush;

    DaqDB::Options options;
    if (!initKvsOptions(options, configFile)) {
        DAQDB_INFO << "Cannot read configuration file [" << configFile << "]"
                   << endl;
        return -1;
    }

    unique_ptr<DaqDB::KVStoreBase> spKVStore;
    try {
        spKVStore.reset(DaqDB::KVStoreBase::Open(options));
    } catch (DaqDB::OperationFailedException &e) {
        DAQDB_INFO << "Failed to create KVStore: " << e.what() << endl;
        return -1;
    }

    map<string, function<bool(DaqDB::KVStoreBase *)>> tests =
        boost::assign::map_list_of("testSyncOperations", testSyncOperations)(
            "testASyncOperations", testAsyncOperations)(
            "testSyncOffloadOperations", testSyncOffloadOperations)(
            "testAsyncOffloadOperations", testAsyncOffloadOperations)(
            "testDhtConnect", testDhtConnect)("testValueSizes", testValueSizes);

    unsigned short failsCount = 0;
    for (auto test : tests) {
        if (!executeTest(test.first, test.second, spKVStore.get())) {
            failsCount++;
        }
    }

    if (failsCount > 0) {
        DAQDB_INFO << format("Test(s) failed [%1%]") % failsCount << endl;
    } else {
        DAQDB_INFO << "All tests passed!" << endl;
    }

    return 0;
}
