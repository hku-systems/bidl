#!/bin/bash
if [ "$#" -ne 2 ]; then
    echo "Usage: ... ./scripts/start_smart.sh <num of nodes> <batch size>"
    exit 1
fi
ant
./scripts/kill_smart.sh
./scripts/remove_tc.sh
rm ./config/currentView

host_num=`cat ./scripts/servers|wc -l`
nodes_per_host=$[$[$1-1]/$[${host_num}-1]]

echo "Generat hosts.config"
i=0
index=0
for host in `cat ./scripts/servers`; do
    if [ $i -eq 0 ] 
    then
        echo ${i}' '${host}' 11000 11001'  > ./config/hosts.config
        let index=$index+1
    else
        for j in $(seq 0 $[${nodes_per_host}-1]); do
            echo ${index}' '${host}' '$[1100+$j]0' '$[1100+$j]1 >> ./config/hosts.config
            let index=$index+1
        done
    fi
    let i=$i+1  
done

echo "Copy system_$1.config to ./config"
cp ./scripts/configs/system_$1.config ./config/system.config

echo "Set batch size to $2"
if [ $2 -eq -1 ]
then
    sed -i "s/system.totalordermulticast.batchtimeout = 2000/system.totalordermulticast.batchtimeout = -1/" ./config/system.config
    sed -i "s/system.totalordermulticast.maxbatchsize = 1/system.totalordermulticast.maxbatchsize = 1000/" ./config/system.config
else
    sed -i "s/system.totalordermulticast.maxbatchsize = 1/system.totalordermulticast.maxbatchsize = $2/" ./config/system.config
fi

./scripts/copy_smart.sh

echo 'Starting '$(cat ./config/hosts.config|wc -l)' nodes...'
let index=0
for host in `cat ./config/hosts.config|awk '{print $2}'`; do
    ssh hkucs@${host} "cd ~/BIDL/bft-smart; ./runscripts/smartrun.sh bftsmart.demo.microbenchmarks.ThroughputLatencyServer ${index} 10 0 0 false nosig rwd >logs/logs_${index}.log 2>&1 &"
    let index=$index+1
done