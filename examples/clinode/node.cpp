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

#include <boost/algorithm/string.hpp>
#include <boost/bind.hpp>
#include <boost/format.hpp>
#include <boost/program_options.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

#include <atomic>
#include <iostream>

#include <daqdb/KVStoreBase.h>

#include "config.h"
#include "debug.h"
#include "nodeCli.h"

using namespace std;
using boost::format;
using namespace boost::algorithm;

namespace po = boost::program_options;

int main(int argc, const char *argv[]) {
    bool interactiveMode = false;
    string configFile;

    std::atomic<int> isRunning;
    logging::add_console_log(std::clog,
                             keywords::format = "%TimeStamp%: %Message%");
    logging::add_common_attributes();
    logging::core::get()->add_thread_attribute("Scope", attrs::named_scope());
    logging::core::get()->set_filter(logging::trivial::severity >=
                                     logging::trivial::error);

    po::options_description argumentsDescription{"Options"};
    argumentsDescription.add_options()("help,h", "Print help messages")(
        "interactive,i", "Enable interactive mode")("log,l", "Enable logging")(
        "config-file,c",
        po::value<string>(&configFile)->default_value("clinode.cfg"),
        "Configuration file");

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
    isRunning = 1;
    options.runtime.shutdownFunc = [&isRunning]() { isRunning = 0; };
    initKvsOptions(options, configFile);

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
