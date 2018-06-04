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

#pragma once

#include <vector>
#include <asio/io_service.hpp>
#include <functional>

#include <FogKV/Types.h>

namespace FogKV {

enum PrimaryKeyAttribute {
	LOCKED = 0x1,
	READY = 0x2,
};

inline PrimaryKeyAttribute operator|(PrimaryKeyAttribute a, PrimaryKeyAttribute b)
{
	return static_cast<PrimaryKeyAttribute>(static_cast<int>(a) | static_cast<int>(b));
}

struct AllocOptions {
};

struct UpdateOptions {
	UpdateOptions() { }
	UpdateOptions(PrimaryKeyAttribute attr) : Attr(attr) { }

	PrimaryKeyAttribute Attr;
};

struct PutOptions {
	void poolerId(unsigned short id)
	{
		_poolerId = id;
	}

	unsigned short poolerId() const
	{
		return _poolerId;
	}

	unsigned short _poolerId = 0;

	enum stages { first, main } stage;
};

struct GetOptions {
	GetOptions() {}
	GetOptions(PrimaryKeyAttribute attr, PrimaryKeyAttribute newAttr) : Attr(attr), NewAttr(newAttr) { }

	PrimaryKeyAttribute Attr;
	PrimaryKeyAttribute NewAttr;
};

struct KeyFieldDescriptor {
	KeyFieldDescriptor() : Size(0), IsPrimary(false) {}
	size_t Size;
	bool IsPrimary;
};

struct KeyDescriptor {
	size_t nfields() const
	{
		return _fields.size();
	}

	void field(size_t idx, size_t size, bool isPrimary = false)
	{
		if (nfields() <= idx)
			_fields.resize(idx + 1);

		_fields[idx].Size = size;
	}
	
	KeyFieldDescriptor field(size_t idx) const
	{
		if (nfields() <= idx)
			return KeyFieldDescriptor();

		return _fields[idx];
	}

	void clear()
	{
		_fields.clear();
	}

	std::vector<KeyFieldDescriptor> _fields;
};

struct RuntimeOptions {
	void io_service(asio::io_service *io_service)
	{
		_io_service = io_service;
	}

	asio::io_service *io_service()
	{
		return _io_service;
	}

	void numOfPoolers(unsigned short count)
	{
		_numOfPoolers = count;
	}

	unsigned short numOfPoolers() const
	{
		return _numOfPoolers;
	}

	std::function<void (std::string)> logFunc = nullptr;

	asio::io_service *_io_service = nullptr;

	unsigned short _numOfPoolers = 1;
};

struct DhtOptions {
	unsigned short Port = 0;
	NodeId Id = 0;
};

struct PMEMOptions {
	std::string Path;
	size_t Size = 0;
};

struct Options {
public:
	Options() { }
	Options(const std::string &path);

	KeyDescriptor Key;
	RuntimeOptions Runtime;
	DhtOptions Dht;

	// TODO move to struct ?
	unsigned short Port = 0;

	std::string KVEngine = "kvtree";
	PMEMOptions PMEM;
};

} // namespace FogKV
