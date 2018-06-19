/**
 * Copyright 2017, Intel Corporation
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

#include <iostream>

#include <asio/io_service.hpp>
#include <boost/bind.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/signal_set.hpp>
#include <boost/program_options.hpp>
#include <boost/format.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/algorithm/string.hpp>

#include <boost/log/trivial.hpp>
#include <boost/log/common.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/attributes/timer.hpp>
#include <boost/log/attributes/named_scope.hpp>
#include <boost/log/sources/logger.hpp>
#include <boost/log/support/date_time.hpp>

#include <FogKV/KVStoreBase.h>

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

BOOST_LOG_INLINE_GLOBAL_LOGGER_DEFAULT(lg, src::severity_logger_mt<logging::trivial::severity_level>)

namespace
{
const unsigned short dhtBackBonePort = 11000;
}

int
main(int argc, const char *argv[])
{
	unsigned short inputPort;
	unsigned short nodeId = 0;
	auto dhtPort = dhtBackBonePort;
	bool interactiveMode = false;
	std::string pmem_path;
	size_t pmem_size;

	logging::add_console_log(std::clog, keywords::format =
			"%TimeStamp%: %Message%");
	logging::add_common_attributes();
	logging::core::get()->add_thread_attribute("Scope", attrs::named_scope());
	logging::core::get()->set_filter(
			logging::trivial::severity >= logging::trivial::error);

#if (1) // Cmd line parsing region
	po::options_description argumentsDescription{"Options"};
        argumentsDescription.add_options()("help,h", "Print help messages")(
            "port,p", po::value<unsigned short>(&inputPort),
            "Node Communication port")("dht,d",
                                       po::value<unsigned short>(&dhtPort),
                                       "DHT Communication port")(
            "nodeid,n", po::value<unsigned short>(&nodeId)->default_value(0),
            "Node ID used to match database file. If not set DB file will be "
            "removed when node stopped.")("interactive,i",
                                          "Enable interactive mode")(
            "log,l", "Enable logging")("pmem-path",
                                       po::value<std::string>(&pmem_path)
                                           ->default_value("/mnt/pmem/pool.pm"),
                                       "Rtree persistent memory pool file")(
            "pmem-size",
            po::value<size_t>(&pmem_size)
                ->default_value(2ull * 1024 * 1024 * 1024),
            "Rtree persistent memory pool size");

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
#endif

	asio::io_service io_service;
	asio::signal_set signals(io_service, SIGINT, SIGTERM);
	signals.async_wait(boost::bind(&asio::io_service::stop, &io_service));

	FogKV::Options options;
	options.Runtime.io_service(&io_service);
	options.Runtime.logFunc = [](std::string msg) {
		BOOST_LOG_SEV(lg::get(), bt::debug) << msg << flush;
	};

	options.Dht.Id = nodeId;
	options.Dht.Port = dhtPort;
	options.Port = inputPort;
	options.PMEM.Path = pmem_path;
	options.PMEM.Size = pmem_size;
	options.Key.field(0, sizeof(KeyType));

	shared_ptr<FogKV::KVStoreBase> spKVStore;
	try {
		spKVStore = shared_ptr<FogKV::KVStoreBase>(FogKV::KVStoreBase::Open(options));
	} catch (FogKV::OperationFailedException &e) {
		cerr << "Failed to create KVStore: " << e.what() << endl;
		return -1;
	}

	BOOST_LOG_SEV(lg::get(), bt::info) <<
			format("DHT node (id=%1%) is running on %2%:%3%") %
				spKVStore->getProperty("fogkv.dht.id") %
				spKVStore->getProperty("fogkv.listener.ip") %
				spKVStore->getProperty("fogkv.listener.port") << flush;
	BOOST_LOG_SEV(lg::get(), bt::info) << format("Waiting for requests on port %1%.") %
			     spKVStore->getProperty("fogkv.listener.dht_port") << flush;

	if (interactiveMode) {
		FogKV::nodeCli nodeCli(spKVStore);
		while (nodeCli()) {
			if (io_service.stopped())
				break;
		}
	} else {
		io_service.run();
	}

	BOOST_LOG_SEV(lg::get(), bt::info) <<
			format("Closing DHT node (id=%1%) on %2%:%3%") %
				spKVStore->getProperty("fogkv.dht.id") %
				spKVStore->getProperty("fogkv.listener.ip") %
				spKVStore->getProperty("fogkv.listener.port") << flush;

	return 0;
}
