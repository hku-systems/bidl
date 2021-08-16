#!/bin/bash

SYS=$1
OP=$2
dst=log/log_${SYS}_${OP}_$(date | tr " " "_" | tr ":" "_")
mkdir -p dst
## fastfabric
if [ $SYS = "fastfabric" ]; then 
    if [ $OP = "performance" ]; then;
        docker stack deploy --compose-file docker-compose-${SYS}.yaml fastfabric
        sleep 2
        doker exec $(docker ps | grep fabric_cli | awk '{print $1}') bash scripts/script.sh
        tape=$(docker ps | grep fabric_tape | awk '{print $`}')
        docker exec $tape rm ACCOUNTS ENDORSEMENT
        docker exec $tape tape --no-e2e -n 80000 --config config.yaml > $dst/simulate.log 2>&1
        docker exec $tape tape --no-e2e -n 80000 --config config.yaml > $dst/order_validation.log 2>&1
        docker stack rm fastfabric
    elif [ $OP = "mvcc" ]; then 
        docker stack deploy --compose-file docker-compose-${SYS}-${OP}.yaml fastfabric-${OP}
        sleep 2
        doker exec $(docker ps | grep fabric_cli | awk '{print $1}') bash scripts/script.sh
        tape=$(docker ps | grep fabric_tape | awk '{print $`}')
        docker exec $tape rm ACCOUNTS ENDORSEMENT
        docker exec $tape tape --e2e -n 50000 --config config.yaml > $dst/log.log 2>&1 # create accounts
        docker exec $tape tape --e2e -n 50000 --config config.yaml > $dst/mvcc.log 2>&1 # transactions
        docker stack rm fastfabric
        cat $dst/mvcc.log | python3 conflict.py

    fi
elif [ $SYS = "fabric" ]; then 

elif [ $SYS = "streamchain" ]; then 
    cd streamchain/setup
    bash run_main.sh 
    cp -r logs/processed $dst/
fi 

