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

#include <asio/io_service.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/signal_set.hpp>
#include <boost/bind.hpp>
#include <boost/program_options.hpp>
#include <daqdb/KVStoreBase.h>
#include <iomanip>
#include <iostream>

#include "config.h"

using namespace std;
namespace po = boost::program_options;

int main(int argc, char **argv) {
    string configFile;
    DaqDB::Options options;

    po::options_description argumentsDescription{"Options"};
    argumentsDescription.add_options()("help,h", "Print help messages")(
        "config-file,c",
        po::value<string>(&configFile)->default_value("mist.cfg"),
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

    initKvsOptions(options, configFile);

    asio::io_service io_service;
    asio::signal_set signals(io_service, SIGINT, SIGTERM);
    signals.async_wait(boost::bind(&asio::io_service::stop, &io_service));

    try {
        DaqDB::KVStoreBase::Open(options);
    } catch (DaqDB::OperationFailedException &e) {
        cerr << "Failed to create KVStore: " << e.what() << endl;
        return -1;
    }
    cout << "DaqDB server running" << endl;

    io_service.run();
    // cleanup code
    cout << "Exiting gracefully" << endl;
    return 0;
}
