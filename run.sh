#!/bin/bash

SYS=$1
OP=$2
dst=log/log_${SYS}_${OP}_$(date | tr " " "_" | tr ":" "_")
mkdir -p dst
## fastfabric
if [ $SYS = "fastfabric" ]; then 
    docker stack deploy --compose-file docker-compose-${SYS}.yaml fabric
    sleep 2
    doker exec $(docker ps | grep fabric_cli | awk '{print $1}') bash scripts/script.sh
    tape=$(docker ps | grep fabric_tape | awk '{print $`}')
    docker exec $tape rm ACCOUNTS ENDORSEMENT
    if [ $OP = "performance" ]; then;
        docker exec $tape tape --no-e2e -n 80000 --config config.yaml > $dst/simulate.log 2>&1
        docker exec $tape tape --no-e2e -n 80000 --config config.yaml > $dst/order_validation.log 2>&1
    elif [ $OP = "mvcc" ]; then 
        docker exec $tape tape --e2e -n 50000 --config config.yaml > $dst/log.log 2>&1 # create accounts
        docker exec $tape tape --e2e -n 50000 --config config.yaml > $dst/mvcc.log 2>&1 # transactions
        cat $dst/mvcc.log | python3 conflict.py
    elif [ $OP = "nondeterminism" ]; then 
        docker exec $tape tape --no-e2e -n 50000 --txtype create_random --config config.yaml > $dst/random.log 2>&1  # create random account
    fi
    docker stack rm fabric
elif [ $SYS = "fabric" ]; then 
    docker stack deploy --compose-file docker-compose-${SYS}.yaml fabric
    sleep 2
    doker exec $(docker ps | grep fabric_cli | awk '{print $1}') bash scripts/script.sh
    tape=$(docker ps | grep fabric_tape | awk '{print $`}')
    docker exec $tape rm ACCOUNTS ENDORSEMENT
    docker exec $tape tape --no-e2e -n 80000 --config config.yaml > $dst/simulate.log 2>&1
    docker exec $tape tape --no-e2e -n 80000 --config config.yaml > $dst/order_validation.log 2>&1
    docker stack rm fastfabric

elif [ $SYS = "streamchain" ]; then 
    cd streamchain/setup
    bash run_main.sh 
    cp -r logs/processed $dst/
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

