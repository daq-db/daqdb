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

#include "CChortAdapter.h"
#include "DhtUtils.h"

#include <iostream>
#include <boost/filesystem.hpp>

#include "ProtocolSingleton.h"

using namespace std;

namespace
{
const string dhtBackBoneIp = "127.0.0.1";
const string dhtOverlayIdentifier = "chordTestBed";
const string rootDirectory = ".";
};

namespace Dht
{

CChortAdapter::CChortAdapter(as::io_service &io_service,
			     unsigned short port)
    : Dht::DhtNode(io_service, port)
{
	/*!
	 * Workaround for cChord library issue.
	 * If following directory not exist then we see segmentation
	 * fault on shutdown (2+ node case)
	 */
	boost::filesystem::path dir(boost::filesystem::current_path());
	dir /= ".chord";
	boost::filesystem::create_directory(dir);
	dir /= "data";
	boost::filesystem::create_directory(dir);

	unsigned short dhtPort = Dht::utils::getFreePort(io_service, port);

	string backBone[] = {
		dhtBackBoneIp,
	};

	node = P_SINGLETON->initChordNode(dhtBackBoneIp, dhtPort, dhtOverlayIdentifier, rootDirectory);

	chord = new Node(backBone[0], port);

	node->join(chord);

	cout << "\n" << node->printStatus();
}

CChortAdapter::~CChortAdapter()
{
	node->shutDown();
	delete node;
	delete chord;

}

} /* namespace Dht */
