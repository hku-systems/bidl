#!/bin/bash 

if [ -d build ]; then 
    echo "already build, skip..."
    exit 0
fi 
mkdir build 
cd build 
cmake -DBUILD_ROCKSDB_STORAGE=TRUE -DCMAKE_CXX_FLAGS='-D SBFT_PAYLOAD_SIZE=32' .. 
make -j
