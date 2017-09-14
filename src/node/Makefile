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

TARGET=dragonNode

# @todo jradtke move to separate makefile for chord dependencies
CCHORD_FOLDER = ../../3rd/cChord
CCHORD_INCLUDE = -I$(CCHORD_FOLDER)/engine/helper\
		-I$(CCHORD_FOLDER)/engine/helper/crypto/\
		-I$(CCHORD_FOLDER)/engine/helper/mongoose_http_server/callbacks\
		-I$(CCHORD_FOLDER)/engine/helper/mongoose_http_server\
		-I$(CCHORD_FOLDER)/engine/helper/thread\
		-I$(CCHORD_FOLDER)/engine/helper/request\
		-I$(CCHORD_FOLDER)/engine/p2p/node\
		-I$(CCHORD_FOLDER)/engine/p2p/protocols/chord\
		-I$(CCHORD_FOLDER)/engine/p2p/protocols\
		-I$(CCHORD_FOLDER)/engine/p2p/transport/http\
		-I$(CCHORD_FOLDER)/engine/p2p/transport
CCHORD_LNK_OPTIONS = -lpthread -lrt -ldl
CCHORD_TARGET = libmymed
CCHORD_LIB = $(CCHORD_FOLDER)/$(CCHORD_TARGET).a

INCLUDE = -I.\
		$(CCHORD_INCLUDE)

CFLAGS += -std=c++11
CFLAGS += -O0 -ggdb

DEPS = $(CCHORD_TARGET)
SRC = dragonNode.cpp
OBJ = $(SRC:.c=.o)
LIB = $(CCHORD_LIB) -lboost_program_options -lboost_system -lboost_filesystem

all: default

default: $(TARGET)

$(CCHORD_TARGET):
	make -C $(CCHORD_FOLDER)

$(TARGET): $(OBJ) | $(DEPS)
	$(CXX) $(CFLAGS) $(CCHORD_LNK_OPTIONS) $(INCLUDE) -o $@ $^ $(LIB)

clean:
	make -C $(CCHORD_FOLDER) clean
	$(RM) -f $(TARGET) *.o

clean-all: clean

.PHONY: clean clean-all default print
