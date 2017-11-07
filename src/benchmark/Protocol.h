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
#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>
#include <string>
#include <sstream>

enum MsgType {
	MSG_NONE,
	MSG_PARAMS,
	MSG_READY,
	MSG_PUT,
	MSG_PUT_RESP,
	MSG_GET,
	MSG_GET_RESP,
};

static inline std::string MsgTypeToString(MsgType type)
{
	switch(type) {
	case MSG_NONE:
		return "MSG_NONE";
	case MSG_PARAMS:
		return "MSG_PARAMS";
	case MSG_READY:
		return "MSG_READY";
	case MSG_PUT:
		return "MSG_PUT";
	case MSG_PUT_RESP:
		return "MSG_PUT_RESP";
	case MSG_GET:
		return "MSG_GET";
	case MSG_GET_RESP:
		return "MSG_GET_RESP";
	default:
		return "UNKNOWN";
	}
}

struct MsgHdr {
	MsgHdr() = default;

	MsgHdr(MsgType type, size_t size) :
		Type((uint64_t)type),
		Size(static_cast<uint64_t>(size))
	{}

	std::string toString()
	{
		std::stringstream ss;
		ss << "{Type: " << MsgTypeToString((MsgType)Type);
		ss << ", Len: " << Size;
		ss << "}";
		return ss.str();
	}

	uint64_t Type;
	uint64_t Size;
};

struct MsgBuffDesc {
	MsgBuffDesc() = default;

	MsgBuffDesc(size_t size, uint64_t addr, uint64_t key) :
		Size(static_cast<uint64_t>(size)),
		Addr(addr),
		Key(key)
	{}

	std::string toString()
	{ 
		std::stringstream ss;
		ss << "{Size: " << Size;
		ss << ", Addr: 0x" << std::hex << Addr;
		ss << ", Key: 0x" << std::hex << Key;
		ss << "}";
		return ss.str();
	}

	uint64_t Size;
	uint64_t Addr;
	uint64_t Key;
};

struct MsgParams {
	MsgParams() = default;

	MsgParams(MsgBuffDesc writeBuff) :
		Hdr(MSG_PARAMS, sizeof(MsgParams)),
		WriteBuff(writeBuff)
	{}

	std::string toString()
	{
		std::stringstream ss;
		ss << "{Hdr: " << Hdr.toString();
		ss << ", WriteBuff: " <<  WriteBuff.toString();
		ss << "}";
		return ss.str();
	}

	MsgHdr Hdr;
	MsgBuffDesc WriteBuff;
};

struct MsgOp {
	MsgOp() = default;

	MsgOp(MsgType type, size_t size) :
		Hdr(type, sizeof(MsgOp)),
		Size(static_cast<uint64_t>(size))
	{}

	std::string toString()
	{
		std::stringstream ss;
		ss << "{Hdr: " << Hdr.toString();
		ss << ", Size: " << Size;
		ss << "}";
		return ss.str();
	}

	MsgHdr Hdr;
	uint64_t Size;
};

struct MsgPut {
	MsgPut() = default;

	MsgPut(size_t keySize, size_t valSize) :
		Hdr(MSG_PUT, sizeof(MsgPut)),
		KeySize(keySize),
		ValSize(valSize)
	{}

	std::string toString()
	{
		std::stringstream ss;
		ss << "{Hdr: " << Hdr.toString();
		ss << ", KeySize: " << KeySize;
		ss << ", ValSize: " << ValSize;
		ss << "}";
		return ss.str();
	}

	MsgHdr Hdr;
	uint64_t KeySize;
	uint64_t ValSize;
};

struct MsgGet {
	MsgGet() = default;

	MsgGet(size_t keySize) :
		Hdr(MSG_GET, sizeof(MsgGet)),
		KeySize(keySize)
	{}

	std::string toString()
	{
		std::stringstream ss;
		ss << "{Hdr: " << Hdr.toString();
		ss << ", KeySize: " << KeySize;
		ss << "}";
		return ss.str();
	}

	MsgHdr Hdr;
	uint64_t KeySize;
};

struct MsgGetResp {
	MsgGetResp() = default;

	MsgGetResp(size_t keySize, size_t valSize) :
		Hdr(MSG_GET_RESP, sizeof(MsgGetResp)),
		KeySize(keySize),
		ValSize(valSize)
	{}

	std::string toString()
	{
		std::stringstream ss;
		ss << "{Hdr: " << Hdr.toString();
		ss << ", ValSize: " << ValSize;
		ss << "}";
		return ss.str();
	}

	MsgHdr Hdr;
	uint64_t KeySize;
	uint64_t ValSize;
};

static inline std::string MsgToString(MsgHdr *hdrp)
{
	switch(hdrp->Type) {
	case MSG_NONE:
		return "MSG_NONE";
	case MSG_PARAMS:
		return ((MsgParams *)hdrp)->toString();
	case MSG_READY:
		return ((MsgHdr *)hdrp)->toString();
	case MSG_PUT:
		return ((MsgPut *)hdrp)->toString();
	case MSG_PUT_RESP:
		return ((MsgOp *)hdrp)->toString();
	case MSG_GET:
		return ((MsgGet *)hdrp)->toString();
	case MSG_GET_RESP:
		return ((MsgGetResp *)hdrp)->toString();
	default:
		return "UNKNOWN";
	}
}


#endif // PROTOCOL_H

