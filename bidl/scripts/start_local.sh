#!/bin/bash
set -u

if [ $# -lt 3 ]; then
    echo "Usage: bash ./bidl/scripts/start_local.sh <1. num of consensus nodes> <2. peak throughput> <3. benchmark> <4. benchmark parameters*>"
    exit 1
fi

script_dir=$(cd "$(dirname "$0")";pwd)
source $script_dir/env.sh

echo "Stopping sequencer/client/consensus/normal nodes..."
bash $script_dir/kill_all_local.sh

echo "Generating hosts.config..."
rm -f $smart_dir/config/hosts.config
for i in `seq 0 $[${1}-1]`; do
    echo ${i} '127.0.0.1' `expr 1100 + ${i}`0  `expr 1100 + ${i}`1 >> $smart_dir/config/hosts.config
done

echo "Generating system.config..."
cp $base_dir/scripts/configs/system_${1}.config $smart_dir/config/system.config

echo "Starting $1 consensus nodes..."
rm -rf $base_dir/logs
mkdir $base_dir/logs
for i in `seq 0 $[${1}-1]`; do
    docker run --name smart$i --net=host --cap-add NET_ADMIN smart:latest bash /home/runscripts/smartrun.sh bftsmart.demo.microbenchmarks.ThroughputLatencyServer $i 100 0 0 false nosig rwd > $base_dir/logs/consensus_${i}.log 2>&1 &
done

echo "Starting the sequencer..., tput cap:$2 kTxns/s."
docker run --name sequencer --net=host sequencer:latest /sequencer/sequencer $2 > $base_dir/logs/sequencer.log 2>&1 &

# echo "Starting normal node..."
# docker run --name normal_node0 --net=host --cap-add NET_ADMIN normal_node:latest /normal_node/server --quiet --tps=$2 --id=0 > $base_dir/logs/normal_0.log 2>&1 &

echo "Waiting for consensus node to start..."
while true; do 
	wait=$( cat $log_dir/consensus_1.log | grep "Ready to process operations" | wc -l)
	if [ $wait -eq 1 ]; then 
		break;
	fi 
	echo "wait 5s for consensus nodes to setup"
	sleep 5
done

echo "benchmarking..."

cd $normal_node_dir
if [ $3 == "test" ]; then
    docker run --name bidl_client --net=host --cap-add NET_ADMIN normal_node /normal_node/client --num=100000 --org=50
elif [ $3 == "performance" ]; then
    docker run --name bidl_client --net=host --cap-add NET_ADMIN normal_node /normal_node/client --num=100000 --org=50
elif [ $3 == "nd" ]; then 
    docker run --name bidl_client --net=host --cap-add NET_ADMIN normal_node /normal_node/client --num=100000 --org=50 --nd=$4
elif [ $3 == "contention" ]; then 
    docker run --name bidl_client --net=host --cap-add NET_ADMIN normal_node /normal_node/client --num=100000 --org=50 --conflict=$4
elif [ $3 == "scalability" ]; then 
    docker run --name bidl_client --net=host --cap-add NET_ADMIN normal_node /normal_node/client --num=100000 --org=50
else 
    echo "Invalid argument."
    exit 1
fi

echo "Please wait..."

bash $base_dir/scripts/get_data.sh
