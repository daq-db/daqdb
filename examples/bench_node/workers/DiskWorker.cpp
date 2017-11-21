/*
 * DiskWorker.cpp
 *
 *  Created on: Nov 2, 2017
 *      Author: jradtke
 */

#include "DiskWorker.h"
#include <iostream>
#include <fstream>
#include <boost/format.hpp>
#include <cerrno>
#include <stdexcept>
#include <sys/stat.h>
#include <cstring>
#include <stdexcept>
#include <string>

#include <log4cxx/basicconfigurator.h>
#include <log4cxx/consoleappender.h>
#include <log4cxx/helpers/exception.h>
#include <log4cxx/logger.h>
#include <log4cxx/simplelayout.h>
#include <log4cxx/xml/domconfigurator.h>

using namespace log4cxx;
using namespace log4cxx::xml;
using namespace log4cxx::helpers;

namespace FogKV {

DiskWorker::DiskWorker(const std::string &diskPath,
		const unsigned int elementSize) :
		_deviceName(diskPath), _valueBlockSize(elementSize) {
	std::ofstream dbFile(_deviceName, std::ios_base::binary);

	if (!dbFile)
		throw(std::runtime_error(
				boost::str(
						boost::format(
								"Cannot open/create file [%1%], errno = %2%")
								% _deviceName % std::strerror(errno))));

	dbFile.close();
}

DiskWorker::~DiskWorker() {
}

off_t DiskWorker::GetFileLength() {
	struct stat st;
	if (stat(_deviceName.c_str(), &st) == -1)
		throw std::runtime_error(std::strerror(errno));
	return st.st_size;
}

std::tuple<KVStatus, unsigned int> DiskWorker::Add(
		const std::vector<char>& value) {

	if (value.size() != _valueBlockSize) {
		LOG4CXX_INFO(log4cxx::Logger::getRootLogger(), "Wrong uffer size");

		return std::make_tuple(KVStatus::FAILED, 0);
	}

	std::ofstream dbFile(_deviceName, std::ios_base::binary | std::ios::app);
	if (!dbFile) {
		LOG4CXX_INFO(log4cxx::Logger::getRootLogger(),
				"Cannot open file to add new element");

		return std::make_tuple(KVStatus::FAILED, 0);
	}

	auto startLba = GetFileLength() / _valueBlockSize;
	dbFile.write(value.data(), value.size() * sizeof(char));

	dbFile.close();

	return std::make_tuple(KVStatus::OK, startLba);
}

KVStatus DiskWorker::Update(const unsigned int lba,
		const std::vector<char>& value) {

	if (value.size() != _valueBlockSize) {
		LOG4CXX_INFO(log4cxx::Logger::getRootLogger(), "Wrong uffer size");

		return KVStatus::FAILED;
	}

	std::fstream dbFile(_deviceName,
			std::ios_base::binary | std::ios::out | std::ios::ate
					| std::ios::in);
	if (!dbFile) {
		LOG4CXX_INFO(log4cxx::Logger::getRootLogger(),
				"Cannot open file to add new element");

		return KVStatus::FAILED;
	}

	auto lastLba = GetFileLength() / _valueBlockSize;

	if (lba >= lastLba) {
		LOG4CXX_INFO(log4cxx::Logger::getRootLogger(),
				"DiskWorker::Update: LBA out of bounds");
		dbFile.close();
		return KVStatus::FAILED;
	}

	dbFile.seekp(lba * _valueBlockSize, std::ios_base::beg);
	dbFile.write(value.data(), value.size() * sizeof(char));

	dbFile.close();

	return KVStatus::OK;
}

KVStatus DiskWorker::Read(const unsigned int lba, std::vector<char>& value) {

	std::ifstream dbFile(_deviceName, std::ios_base::binary | std::ios::ate);

	if (!dbFile) {
		LOG4CXX_INFO(log4cxx::Logger::getRootLogger(),
				"Cannot open file to get element");

		return KVStatus::FAILED;
	}

	auto lastLba = GetFileLength() / _valueBlockSize;

	if (lba >= lastLba) {
		LOG4CXX_INFO(log4cxx::Logger::getRootLogger(),
				"DiskWorker::Read: LBA out of bounds");
		dbFile.close();
		return KVStatus::FAILED;
	}

	value.resize(_valueBlockSize);
	dbFile.seekg(lba * _valueBlockSize, std::ios_base::beg);
	dbFile.read(value.data(), _valueBlockSize * sizeof(char));

	dbFile.close();

	return KVStatus::OK;
}

std::tuple<float, float> DiskWorker::getIoStat() {
	return _ioMeter.getIoStat();
}

}
