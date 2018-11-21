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

#pragma once

#include <asio/io_service.hpp>
#include <daqdb/KVStoreBase.h>
#include <daqdb/Options.h>

namespace DaqDB {

class Env {
  public:
    Env(const DaqDB::Options &options, KVStoreBase* parent);
    virtual ~Env();

    asio::io_service &ioService();
    inline size_t keySize() { return _keySize; }
    const Options &getOptions();
    inline std::string getZhtConfFile() { return _zhtConfFile; };
    inline std::string getZhtNeighborsFile() { return _zhtNeighborsFile; };
    inline std::string getSpdkConfFile() { return _spdkConfFile; };

    void createZhtConfFiles();
    void removeZhtConfFiles();
    void createSpdkConfFiles();
    void removeSpdkConfFiles();

    KVStoreBase *getKvs() { return _parent; }

  private:
    asio::io_service _ioService;
    size_t _keySize;
    Options _options;

    std::string _zhtConfFile;
    std::string _zhtNeighborsFile;
    std::string _spdkConfFile;

    KVStoreBase* _parent;
};

} // namespace DaqDB
