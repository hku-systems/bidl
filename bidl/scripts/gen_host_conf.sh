#!/bin/bash -e

if [ "$#" -lt 1 ]; then
    echo "Usage: bash ./scripts/gen_host_conf.sh <num of consensus nodes>"
    exit 1
fi

script_dir=$(cd "$(dirname "$0")";pwd)
source $script_dir/env.sh

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

cat $smart_dir/config/hosts.config