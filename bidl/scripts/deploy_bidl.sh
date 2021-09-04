#!/bin/bash
set -u

if [ "$#" -lt 1 ]; then
    echo "Usage: bash ./bidl/scripts/deploy_bidl.sh <1. num of consensus nodes>"
    exit 1
fi


script_dir=$(cd "$(dirname "$0")";pwd)
source $script_dir/env.sh

source $base_dir/scripts/kill_all.sh

echo "Generating hosts.config"
host_num=`cat $base_dir/scripts/servers|wc -l`
if [ $host_num -gt $1 ]; then
    host_num=$1
fi
consensus_nodes_per_host=$[$1/$host_num]
extra_nodes=$[$1 - $host_num*$consensus_nodes_per_host]

rm -f $smart_dir/config/hosts.config
touch $smart_dir/config/hosts.config
i=0
index=0
for host in `cat $base_dir/scripts/servers`; do
    for i in $(seq 0 $[${consensus_nodes_per_host}-1]); do
        echo ${index}' '${host}' '$[1100+$i]0' '$[1100+$i]1 >> $smart_dir/config/hosts.config
        let index=$index+1
    done
    if [ $index -eq $1 ]; then
        break
    fi
done

i=$consensus_nodes_per_host
for node in `seq 0 $[${extra_nodes}-1]`; do
    echo ${index}' '${host}' '$[1100+$i]0' '$[1100+$i]1 >> $smart_dir/config/hosts.config
    let i=$i+1
    let index=$index+1
done


echo "Copy system_$1.config to $(pwd)/config/system.config"
cp $base_dir/scripts/configs/system_$1.config $smart_dir/config/system.config

echo "Building images"
bash $base_dir/scripts/build_image.sh

bash $base_dir/scripts/copy_smart_docker.sh
