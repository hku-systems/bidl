#!/bin/bash -e
if [ "$#" -ne 1 ]; then
    echo "Usage: bash ./scripts/start_smart_docker.sh <num of nodes>"
    exit 1
fi

script_dir=$(cd "$(dirname "$0")";pwd)
source $script_dir/env.sh

host_num=`cat $base_dir/scripts/servers|wc -l`
nodes_per_host=$[$[$1-1]/$[${host_num}-1]]
echo "Nodes per host is $nodes_per_host"

echo "######################################################"
echo "Deploy $1 consensus nodes on $host_num servers"
echo "######################################################"

echo "Generating hosts.config"
i=0
index=0
for host in `cat $base_dir/scripts/servers`; do
    if [ $i -eq 0 ] 
    then
        echo ${i}' '${host}' 11000 11001'  > $smart_dir/config/hosts.config
        let index=$index+1
    else
        for j in $(seq 0 $[${nodes_per_host}-1]); do
            echo ${index}' '${host}' '$[1100+$j]0' '$[1100+$j]1 >> $smart_dir/config/hosts.config
            let index=$index+1
        done
    fi
    let i=$i+1  
done

# echo "Copy system_$1.config to $(pwd)/config/system.config"
cp $base_dir/scripts/configs/system_$1.config $smart_dir/config/system.config

source $base_dir/compile.sh

echo "Building image"
source $base_dir/scripts/build_image.sh

# stop running containers on all hosts, must stop before deploy, or the old image cannot be deleted
source $base_dir/scripts/kill_all.sh

# deploy to all hosts
source $base_dir/scripts/copy_smart_docker.sh

echo 'Starting '$(cat $smart_dir/config/hosts.config|wc -l)' nodes...'
while read line; do
    node=$(echo $line | awk '{print $1}')
    host=$(echo $line | awk '{print $2}')
    echo ${node} ${host}
    ssh -n jqi@${host} "cd ~; rm -rf logs; mkdir logs; docker run --name smart${node} --net=host --cap-add NET_ADMIN smart bash /home/runscripts/smartrun.sh bftsmart.demo.microbenchmarks.ThroughputLatencyServer ${node} 10 0 0 false nosig rwd > logs/log_${node}.log 2>&1 &"
done < $smart_dir/config/hosts.config

echo "Starting the sequencer..."
$sequencer_dir/sequencer &> sequencer.log &

echo "Starting normal node..."
docker run --name normal_node --net=host --cap-add NET_ADMIN normal_node /normal_node/server > normal.log 2>&1 &