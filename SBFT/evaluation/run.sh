#!/bin/bash

if [ $# -ne 4 ]; then
    echo "Usage: bash evaluation/run.sh nodes f clients ops"
    exit 1
fi

nodes=$1
f=$2
clients=$3
ops=$4

# stop 
bash evaluation/stop.sh
sleep 10

# Generate keys 
./build/tools/GenerateConcordKeys -n $nodes -f $f -o private_replica_

# Run replicas
for i in `seq 0 $((nodes-1))`; do
    echo "start replica $i"
    ./build/tests/simpleTest/server -id $i -r $nodes -c $clients > log_$i &
done

# Wait for replicas to be ready
sleep 30

# Run clients
for i in `seq $nodes $((clients+nodes-2))`; do
    echo "start client $i"
    ./build/tests/simpleTest/client -id $i -r $nodes -f $f -cl $clients -i $ops > log_client_$i &
done

i=$((clients+nodes-1))
echo "start client $i"
./build/tests/simpleTest/client -id $i -r $nodes -f $f -cl $clients -i $ops > log_client_$i 
