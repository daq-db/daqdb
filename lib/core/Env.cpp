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

#include <asio/io_service.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

#include "Env.h"
#include <Logger.h>

namespace DaqDB {

const size_t DEFAULT_KEY_SIZE = 16;

const std::string DEFAULT_ZHT_CONF_FILE = "zht.conf";
const std::string DEFAULT_ZHT_NEIGHBOR_FILE = "neighbors.conf";
const std::string DEFAULT_SPDK_CONF_FILE = "spdk.conf";

Env::Env(const Options &options, KVStoreBase *parent)
    : _options(options), _parent(parent), _keySize(0),
      _zhtConfFile(DEFAULT_ZHT_CONF_FILE),
      _zhtNeighborsFile(DEFAULT_ZHT_NEIGHBOR_FILE),
      _spdkConfFile(DEFAULT_SPDK_CONF_FILE) {
    for (size_t i = 0; i < options.Key.nfields(); i++)
        _keySize += options.Key.field(i).Size;
    if (_keySize)
        _keySize = DEFAULT_KEY_SIZE;
}

Env::~Env() {}

asio::io_service &Env::ioService() { return _ioService; }

const Options &Env::getOptions() { return _options; }

void Env::createZhtConfFiles() {
    if (!boost::filesystem::exists(_zhtConfFile)) {
        std::ofstream confOut(_zhtConfFile, std::ios::out);
        confOut << "PROTOCOL TCP" << std::endl;
        confOut << "PORT " << _options.Dht.port << std::endl;
        confOut << "MSG_MAXSIZE " << _options.Dht.msgMaxsize << std::endl;
        confOut << "SCCB_POLL_INTERVAL " << _options.Dht.sccbPoolInterval
                << std::endl;
        confOut << "INSTANT_SWAP " << _options.Dht.instantSwap << std::endl;
        confOut.close();
        DAQ_DEBUG("ZHT configuration file created");
    }
    if (!boost::filesystem::exists(_zhtNeighborsFile)) {
        std::ofstream neighbourOut(_zhtNeighborsFile, std::ios::out);
        for (auto neighbor : _options.Dht.neighbors) {
            neighbourOut << neighbor->ip << " " << neighbor->port << std::endl;
        }
        neighbourOut.close();
        DAQ_DEBUG("ZHT neighbor file created");
    }
}

void Env::removeZhtConfFiles() {
    if (boost::filesystem::exists(_zhtConfFile)) {
        boost::filesystem::remove(_zhtConfFile);
    }
    if (boost::filesystem::exists(_zhtNeighborsFile)) {
        boost::filesystem::remove(_zhtNeighborsFile);
    }
}

void Env::createSpdkConfFiles() {
    if (!boost::filesystem::exists(_spdkConfFile)) {
        if (!_options.Offload.nvmeAddr.empty() &&
            !_options.Offload.nvmeName.empty()) {
            std::ofstream spdkConf(_spdkConfFile, std::ios::out);

            spdkConf << "[Nvme]" << std::endl
                     << "  TransportID \"trtype:PCIe traddr:"
                     << _options.Offload.nvmeAddr << "\" "
                     << _options.Offload.nvmeName << std::endl;
            spdkConf.close();
            DAQ_DEBUG("ZHT neighbor file created");
        }
    }
}

void Env::removeSpdkConfFiles() {
    if (boost::filesystem::exists(_spdkConfFile)) {
        boost::filesystem::remove(_spdkConfFile);
    }
}

} // namespace DaqDB
