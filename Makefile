.PHONY: all clean install
default:    all
all:    
    scons
clean:
    scons -c
install:
    scons install
