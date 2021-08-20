#!/bin/bash

SYS=$1
OP=$2
## fastfabric
if [ $SYS = "fastfabric" ]; then 
    bash run-ff.sh $OP
elif [ $SYS = "fabric" ]; then 
    bash run-fabric.sh 
elif [ $SYS = "streamchain" ]; then 
    bash run-streamchain.sh
elif [ $SYS = "hotstuff" ]; then 
    cd hotstuff
    bash docker/run.sh hotstuff1.0
    docker exec bash scripts/deploy/run.sh setup  
    # set payload size in scripts/doploy/group_vars/all.yml in container c0
    docker exec bash gen_all.sh 
    docker exec bash run.sh new run1
    docker exec bash run_cli.sh new run1_cli
    sleep 10s
    docker exec bash run_cli.sh stop run1_cli
    docker exec bash run_cli.sh fetch run1_cli 
    docker exec cat run1_cli/remote/*/stderr | python3 ../thr_hist.py --plot
    bash docker/kill_docker.sh 
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

