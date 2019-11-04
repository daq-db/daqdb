#!/bin/sh -e

OLD_PATH=$PATH
PATH=/usr/bin:$PATH
autoreconf --install --symlink -f
PATH=$OLD_PATH 

libdir() {
        echo $(cd $1/$(gcc -print-multi-os-directory); pwd)
}

args="--prefix=/usr --libdir=$(libdir /usr/lib)"

echo
echo "----------------------------------------------------------------"
echo "Initialized build system. For a common configuration please run:"
echo "----------------------------------------------------------------"
echo
echo "./configure $args"
echo
