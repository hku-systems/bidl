#!/bin/bash

if [ "$#" -ne 1 ]; then
    echo "Usage: ... ./scripts/local_start_docker.sh <num of nodes>"
    exit 1
fi

echo "Stopping docker nodes..."
docker stop $(docker ps -aq --filter name=smart)
docker rm $(docker ps -aq --filter name=smart)
docker stop $(docker ps -aq --filter name=multicast)
docker rm $(docker ps -aq --filter name=multicast)

# echo "Compiling..."
# ant

echo "Generating hosts.config..."
rm -f ./config/hosts.config
for i in `seq 0 $[${1}-1]`; do
    echo ${i} '127.0.0.1' `expr 1100 + ${i}`0  `expr 1100 + ${i}`1 >> ./config/hosts.config
done

echo "Rebuilding image..."
cp ./scripts/configs/system_${1}.config ./config/system.config
rm -f config/currentView
rm -f smart.tar
rm -rf logs
docker image build -t smart .
mkdir logs

echo "Start $1 local nodes..."

for i in `seq 0 $[${1}-1]`; do
    docker run --name smart$i --net=host --cap-add NET_ADMIN smart bash /home/runscripts/smartrun.sh bftsmart.demo.microbenchmarks.ThroughputLatencyServer $i 50 0 0 false nosig rwd >logs/log_${i}.log 2>&1 &
done