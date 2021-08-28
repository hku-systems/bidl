#!/bin/bash

if [ $# -ne 2 ]; then
    echo "Usage: bash evaluation/run.sh $clients $ops"
    exit 1
fi

nodes=4
f=1
clients=$1
ops=$2

# stop 
bash evaluation/stop.sh
sleep 10

# Generate keys 
./build/tools/GenerateConcordKeys -n $nodes -f $f -o private_replica_

# Run replicas
for i in `seq 0 $((nodes-1))`; do
    echo "start replica $i"
    ./build/tests/simpleTest/server -id $i -c $clients > log_$i &
done

# Wait for replicas to be ready
sleep 5

# Run clients
for i in `seq $nodes $((clients+nodes-2))`; do
    echo "start client $i"
    ./build/tests/simpleTest/client -id $i -cl $clients -i $ops > log_client_$i &
done

i=$((clients+nodes-1))
echo "start client $i"
./build/tests/simpleTest/client -id $i -cl $clients -i $ops > log_client_$i 
