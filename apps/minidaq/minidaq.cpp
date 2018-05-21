/**
 * Copyright 2018, Intel Corporation
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
#include <boost/program_options.hpp>

#include "FogKV/KVStoreBase.h"
#include "MinidaqReadoutNode.h"

using namespace std;

namespace po = boost::program_options;

static FogKV::KVStoreBase* openKVS(std::string &pmemPath, size_t pmemSize)
{
	// todo remove redundant stuff once initialization is fixed
	FogKV::Options options;
	options.PMEM.Path = pmemPath;
	options.PMEM.Size = pmemSize;
	options.Port = 0;
	options.Dht.Port = 0;
	options.Dht.Id = 0;
	options.Runtime._io_service = nullptr;
	options.Key.field(0, sizeof(FogKV::MinidaqKey::event_id), true);
	options.Key.field(1, sizeof(FogKV::MinidaqKey::subdetector_id));
	options.Key.field(2, sizeof(FogKV::MinidaqKey::run_id));

	return FogKV::KVStoreBase::Open(options);
}

int main(int argc, const char *argv[])
{
	bool readoutMode = false;
	FogKV::KVStoreBase *kvs;
	std::string pmem_path;
	int rampUpSec = 1;
	size_t pmem_size;
	int nSec = 10;
	int nTh = 1;

	po::options_description argumentsDescription{"Options"};
	argumentsDescription.add_options()
			("help,h", "Print help messages")
			("readout,r", "Run in readout mode")
			("threads,t", po::value<int>(&nTh), "Number of worker threads")
			("duration,d", po::value<int>(&nSec), "Desired test duration in seconds")
			("ramp", po::value<int>(&rampUpSec), "Desired ramp up time in seconds")
			("pmem-path", po::value<std::string>(&pmem_path)->default_value("/mnt/pmem/pmemkv.dat"), "pmemkv persistent memory pool file")
			("pmem-size", po::value<size_t>(&pmem_size)->default_value(512 * 1024 * 1024), "pmemkv persistent memory pool size")
			;

	po::variables_map parsedArguments;
	try {
		po::store(po::parse_command_line(argc, argv,
						 argumentsDescription),
			  parsedArguments);

		if (parsedArguments.count("help")) {
			std::cout << argumentsDescription << endl;
			return 0;
		}
		if (parsedArguments.count("readout")) {
			readoutMode = true;
		}

		po::notify(parsedArguments);
	} catch (po::error &parserError) {
		cerr << "Invalid arguments: " << parserError.what() << endl
		     << endl;
		cerr << argumentsDescription << endl;
		return -1;
	}

	try {
		kvs = openKVS(pmem_path, pmem_size);

		FogKV::MinidaqReadoutNode nodeReadout(kvs);

		// Start workers
		if (readoutMode) {
			nodeReadout.SetDuration(nSec);
			nodeReadout.SetRampUp(rampUpSec);
			nodeReadout.SetThreads(nTh);
			nodeReadout.Run();
		}

		// Wait for results
		if (readoutMode) {
			nodeReadout.Wait();
		}

		// Show results
		if (readoutMode) {
			cout << "### READOUT NODES ###" << std::endl;
			nodeReadout.Show();
		}
	} catch (std::exception &e) {
		std::cerr << e.what() << std::endl;
		return 0;
	}

	return 0;
}
