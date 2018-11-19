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

#include <config/Configuration.h>
#include <libconfig.h++>

using namespace libconfig;

namespace DaqDB {

bool readConfiguration(const std::string &configFile, DaqDB::Options &options) {
    std::stringstream ss;
    return readConfiguration(configFile, options, ss);
}

bool readConfiguration(const std::string &configFile, DaqDB::Options &options,
                       std::stringstream &ss) {
    Config cfg;
    try {
        cfg.readFile(configFile.c_str());
    } catch (const libconfig::FileIOException &fioex) {
        ss << "I/O error while reading file.\n";
        return false;
    } catch (const libconfig::ParseException &pex) {
        ss << "Parse error at " << pex.getFile() << ":" << pex.getLine()
           << " - " << pex.getError();
        return false;
    }

    cfg.lookupValue("pmem_path", options.PMEM.poolPath);
    long long pmemSize;
    if (cfg.lookupValue("pmem_size", pmemSize))
        options.PMEM.totalSize = pmemSize;
    int allocUnitSize;
    if (cfg.lookupValue("alloc_unit_size", allocUnitSize))
        options.PMEM.allocUnitSize = allocUnitSize;

    // Configure key structure
    std::string primaryKey;
    cfg.lookupValue("primaryKey", primaryKey);
    std::vector<int> keysStructure;
    const libconfig::Setting &keys_settings = cfg.lookup("keys_structure");
    for (int n = 0; n < keys_settings.getLength(); ++n) {
        keysStructure.push_back(keys_settings[n]);
        // TODO extend functionality of primary key definition
        options.Key.field(n, keysStructure[n], (n == 0) ? true : false);
    }

    cfg.lookupValue("protocol", options.Dht.protocol);
    int port;
    if (cfg.lookupValue("port", port))
        options.Dht.port = port;
    long long msgMaxSize;
    if (cfg.lookupValue("msg_maxsize", msgMaxSize))
        options.Dht.msgMaxsize = msgMaxSize;
    int sccbPoolInterval;
    if (cfg.lookupValue("sccb_pool_interval", sccbPoolInterval))
        options.Dht.sccbPoolInterval = sccbPoolInterval;
    int instantSwap;
    if (cfg.lookupValue("instant_swap", instantSwap))
        options.Dht.instantSwap = instantSwap;

    int offloadAllocUnitSize;
    if (cfg.lookupValue("offload_unit_alloc_size", offloadAllocUnitSize))
        options.Offload.allocUnitSize = offloadAllocUnitSize;
    cfg.lookupValue("offload_nvme_addr", options.Offload.nvmeAddr);
    cfg.lookupValue("offload_nvme_name", options.Offload.nvmeName);
    std::string range_mask;
    cfg.lookupValue("dht_key_mask", range_mask);

    try {
        const libconfig::Setting &neighbors = cfg.lookup("neighbors");
        for (int n = 0; n < neighbors.getLength(); ++n) {
            auto dhtNeighbor = new DaqDB::DhtNeighbor;
            const libconfig::Setting &neighbor = neighbors[n];
            dhtNeighbor->ip = neighbor["ip"].c_str();
            dhtNeighbor->port = (unsigned int)(neighbor["port"]);
            dhtNeighbor->keyRange.mask = range_mask;
            dhtNeighbor->keyRange.start = neighbor["keys"]["start"].c_str();
            dhtNeighbor->keyRange.end = neighbor["keys"]["end"].c_str();
            options.Dht.neighbors.push_back(dhtNeighbor);
        }
    } catch (SettingNotFoundException &e) {
        // no action needed
    }
//--get eRPC configuration---------------------------------------------------
	std::string eRPCserver;
	if (cfg.lookupValue("ServerHostname", eRPCserver))
		options.eRPC.ServerHostname = eRPCserver;
	std::string eRPCclient;
	if (cfg.lookupValue("ClientHostname", eRPCclient))
		options.eRPC.ClientHostname = eRPCclient;
	int eRPCport;
	if (cfg.lookupValue("UDPPort", eRPCport))
		options.eRPC.UDPPort = eRPCport;
	int eRPCreq;
	if (cfg.lookupValue("ReqType", eRPCreq))
		options.eRPC.ReqType = eRPCreq;
	int eRPCsize;
	if (cfg.lookupValue("MsgSize", eRPCsize))
		options.eRPC.MsgSize = eRPCsize;
//--END----------------------------------------------------------------------
    return true;
}

} // namespace DaqDB
