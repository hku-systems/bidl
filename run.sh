#!/bin/bash

SYS=$1
OP=$2
## fastfabric
if [ $SYS = "fastfabric" ]; then 
    bash run-ff.sh $OP $3
elif [ $SYS = "fabric" ]; then 
    bash run-fabric.sh 
elif [ $SYS = "streamchain" ]; then 
    bash run-streamchain.sh
elif [ $SYS = "hotstuff" ]; then 
    bash run-hotstuff.sh 
elif [ $SYS = "SBFT" ]; then 
    cd SBFT
    docker run -it --rm --name sbfthost -v $PWD:/home/SBFT sbft:base
    # in docker container 
    # mkdir build && cd build 
    # cmake -DBUILD_ROCKSDB_STORAGE=TRUE -DBUILD_COMM_TCP_PLAIN=TRUE
    # -DCMAKE_CXX_FLAGS='-D SBFT_PAYLOAD_SIZE=1024' .. 
    # make -j
    # cd ..
    # bash evaluation/run.sh 150 500
    # cat log_client* | python3 evaluation/tput.py
    # bash evaluation/stop.sh
fi 

