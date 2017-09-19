#
# Copyright 2017, Intel Corporation
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in
#       the documentation and/or other materials provided with the
#       distribution.
#
#     * Neither the name of the copyright holder nor the names of its
#       contributors may be used to endorse or promote products derived
#       from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

TARGET=dragon

DHT_FOLDER = ../dht
DHT_INCLUDE = -I$(DHT_FOLDER)
DHT_LIB = $(DHT_FOLDER)/dht.a
THIRD_PARTY_FOLDER = ../../3rd
DHT_IMPL_TARGET = libmymed
DHT_IMPL_FOLDER = $(THIRD_PARTY_FOLDER)/cChord
DHT_IMPL_INCLUDE = -I$(DHT_IMPL_FOLDER)/engine/helper\
		-I$(DHT_IMPL_FOLDER)/engine/helper/crypto/\
		-I$(DHT_IMPL_FOLDER)/engine/helper/mongoose_http_server/callbacks\
		-I$(DHT_IMPL_FOLDER)/engine/helper/mongoose_http_server\
		-I$(DHT_IMPL_FOLDER)/engine/helper/thread\
		-I$(DHT_IMPL_FOLDER)/engine/helper/request\
		-I$(DHT_IMPL_FOLDER)/engine/p2p/node\
		-I$(DHT_IMPL_FOLDER)/engine/p2p/protocols/chord\
		-I$(DHT_IMPL_FOLDER)/engine/p2p/protocols\
		-I$(DHT_IMPL_FOLDER)/engine/p2p/transport/http\
		-I$(DHT_IMPL_FOLDER)/engine/p2p/transport
DHT_IMPL_LIB = -lpthread -lrt -ldl $(DHT_IMPL_FOLDER)/$(DHT_IMPL_TARGET).a

CFLAGS += -std=c++11 -O0 -ggdb
SRC = dragon.cpp
OBJ = $(SRC:.c=.o)

INCLUDE = -I. $(DHT_IMPL_INCLUDE) $(DHT_INCLUDE)
DEPS = $(DHT_TARGET) $(DHT_LIB)
LIB = -lboost_program_options -lboost_system -lboost_filesystem $(DHT_LIB) $(DHT_IMPL_LIB)

all: default

default: clean $(TARGET)

$(DHT_IMPL_TARGET):
	make -C $(DHT_IMPL_FOLDER)
$(DHT_LIB): $(DHT_IMPL_TARGET)
	make -C $(DHT_FOLDER)

$(TARGET): $(OBJ) | $(DEPS)
	$(CXX) $(CFLAGS) $(DHT_IMPL_LNK_OPTIONS) $(INCLUDE) -o $@ $^ $(LIB)

clean:
	$(RM) -f $(TARGET) *.o

.PHONY: clean default
