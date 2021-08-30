#!/bin/bash -e

if [ "$#" -ne 2 ]; then
    echo "Usage: bash ./scripts/local_start_docker.sh <num of nodes> <submit tput>"
    exit 1
fi

script_dir=$(cd "$(dirname "$0")";pwd)
source $script_dir/env.sh

echo "Stopping sequencer/consensus/normal nodes..."
source $script_dir/kill_all_local.sh

# echo "Generating hosts.config.."
rm -f $smart_dir/config/hosts.config
for i in `seq 0 $[${1}-1]`; do
    echo ${i} '127.0.0.1' `expr 1100 + ${i}`0  `expr 1100 + ${i}`1 >> $smart_dir/config/hosts.config
done

# echo "Generating system.config..."
cp $base_dir/scripts/configs/system_${1}.config $smart_dir/config/system.config

source $base_dir/scripts/compile.sh
source $base_dir/scripts/build_image.sh

echo "Start $1 consensus nodes..."
rm -rf $base_dir/logs
mkdir $base_dir/logs
for i in `seq 0 $[${1}-1]`; do
    docker run --name smart$i --net=host --cap-add NET_ADMIN smart bash /home/runscripts/smartrun.sh bftsmart.demo.microbenchmarks.ThroughputLatencyServer $i 10 0 0 false nosig rwd > $base_dir/logs/log_${i}.log 2>&1 &
done

echo "Starting the sequencer..., tput:$2 kTxns/s."
$sequencer_dir/sequencer $2 &> $base_dir/logs/sequencer.log &

echo "Starting normal node..."
docker run --name normal_node --net=host --cap-add NET_ADMIN normal_node /normal_node/server --quiet --id=0 > $base_dir/logs/normal.log 2>&1 &
# docker run --name normal_node --net=host --cap-add NET_ADMIN normal_node /normal_node/server --quiet > $base_dir/logs/normal.log 2>&1 &

sleep 10
echo "benchmarking..."
cd $normal_node_dir
go run ./cmd/client --num=100000 --org=50

cd $base_dir
echo "Please wait..."
sleep 10
source $base_dir/scripts/get_data.sh
