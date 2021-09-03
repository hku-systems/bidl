#!/bin/bash
set -u

if [ "$#" -lt 2 ]; then
    echo "Usage: bash ./bidl/scripts/start_bidl_only_normal.sh <1. num of normal nodes> <2. peak throughput>"
    exit 1
fi

script_dir=$(cd "$(dirname "$0")";pwd)
source $script_dir/env.sh

################################################################################

host_num=`cat $base_dir/scripts/servers|wc -l`
if [ $host_num -gt $1 ]; then
    host_num=$1
fi
normal_nodes_per_host=$[$1/$host_num]

echo "Normal nodes per host is $normal_nodes_per_host"
echo "######################################################"
echo "Start $1 normal nodes on $host_num servers"
echo "######################################################"

i=0
for host in `cat $base_dir/scripts/servers`; do
    for node in `seq 0 $[${normal_nodes_per_host}-1]`; do
        echo ${host} ${i}
        ssh -n ${USER}@${host} "docker run --name normal_node$i --net=host --cap-add NET_ADMIN normal_node /normal_node/server --quiet --tps=$2 --id=$i > logs/normal_${i}.log 2>&1 &"
        let i=$i+1
    done
done 
