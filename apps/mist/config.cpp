/**
 * Copyright 2017-2018, Intel Corporation
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

#include "config.hpp"

Configuration::Configuration(const char * file) : fileName(file){
	try {
		cfg.readFile(file);
	}
	catch(const FileIOException &fioex) {
		std::cerr << "I/O error while reading file." << std::endl;
	}
	catch(const ParseException &pex) {
		std::cerr << "Parse error at " << pex.getFile() << ":" << pex.getLine()
        		      << " - " << pex.getError() << std::endl;
	}
}

void Configuration::readConfiguration(void) {
	// required parameters
	string mode;
	int port;
	cfg.lookupValue("mode", mode);
	cfg.lookupValue("port", port);

	string primaryHash, replicaHash, primaryKey;
	vector<int> keysStructure;
	cfg.lookupValue("primaryHash", primaryHash);
	cfg.lookupValue("replicaHash", replicaHash);
	const Setting &keys_settings = cfg.lookup("keys_structure");
	for (int n = 0; n < keys_settings.getLength(); ++n)
		keysStructure.push_back(keys_settings[n]);
	cfg.lookupValue("primaryKey", primaryKey);

	// optional parameters
	string pmem_path = "";
	int pmem_size = 0;
	string loggingLevel = "WARN";
	cfg.lookupValue("pmem_path", pmem_path);
	cfg.lookupValue("pmem_size", pmem_size);
	cfg.lookupValue("logging_level", loggingLevel);

	cout << "FogKV/mode=" << mode << "; file=" << pmem_path
				<< "; size=" << pmem_size
				<< endl;
	cout << "keys structure=";
	for (auto n: keysStructure)
		cout << n << " ";
	cout << endl;
}
