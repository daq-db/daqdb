TARGET=dragon-node

CFLAGS += -std=c++11
CFLAGS += -O0 -ggdb

SRC = dragon-node.cpp
OBJ = $(SRC:.c=.o)

all: default

default: $(TARGET)

$(TARGET): $(OBJ) | $(DEPS)
	$(CXX) $(CFLAGS) -o $@ $^

clean:
	$(RM) -f $(TARGET) *.o

clean-all: clean

.PHONY: clean clean-all default print
