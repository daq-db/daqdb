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

LIBRARY_NAME=dht

include dht_impl.mk

INCLUDE = -I. $(DHT_IMPL_INCLUDE)

CFLAGS += -std=c++11
CFLAGS += -O0 -ggdb

SRC = $(wildcard *.cpp)
OBJ = $(SRC:.c=.o)
LIB = -lboost_program_options -lboost_system -lboost_filesystem $(DHT_IMPL_LIB)

all: default

default: $(LIBRARY_NAME)

$(LIBRARY_NAME): $(OBJ)
	$(CXX) $(CFLAGS) $(DHT_IMPL_LNK_OPTIONS) $(INCLUDE) -c $^
	rm -f $(LIBRARY_NAME).a
	ar rcs $(LIBRARY_NAME).a *.o
	ranlib $(LIBRARY_NAME).a

clean:
	rm -f $(LIBRARY_NAME).a
	$(RM) -f $(TARGET) *.o

clean-all: clean

.PHONY: clean clean-all default print