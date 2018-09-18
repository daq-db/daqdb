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
#include <iomanip>
#include "config.h"
#include <daqdb/KVStoreBase.h>
#include <asio/io_service.hpp>
#include <boost/bind.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/signal_set.hpp>


int main(int argc, char ** argv) {
	// TODO utilize boost::program_options for help on each parameter
	if (argc != 2) {
		cout << "Please specify configuration file, e.g. ./mist myConfig.cfg" << endl;
		return -1;
	}

	Configuration fogServerConfiguration(argv[1]);

	Options options;
	fogServerConfiguration.readConfiguration(options);

	asio::io_service io_service;
	asio::signal_set signals(io_service, SIGINT, SIGTERM);
	signals.async_wait(boost::bind(&asio::io_service::stop, &io_service));

	try {
		KVStoreBase::Open(options);
	} catch (OperationFailedException &e) {
		cerr << "Failed to create KVStore: " << e.what() << endl;
		return -1;
	}
	cout << "FogKV server running" << endl;

	io_service.run();
	//cleanup code
	cout << "Exiting gracefuly" << endl;
	return 0;
}
