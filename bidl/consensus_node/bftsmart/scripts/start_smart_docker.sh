#!/bin/bash
if [ "$#" -ne 1 ]; then
    echo "Usage: ... ./scripts/start_smart_docker.sh <num of nodes>"
    exit 1
fi

host_num=`cat ./scripts/servers|wc -l`
nodes_per_host=$[$[$1-1]/$[${host_num}-1]]

echo "######################################################"
echo "Starting $1 nodes on $host_num servers"
echo "######################################################"

# stop running containers on all servers, must stop before deploy, or the old image cannot be deleted
./scripts/kill_smart_docker.sh


echo 'Starting '$(cat ./scripts/servers|wc -l)' nodes...'
let index=0
for host in `cat ./scripts/servers`; do
    ssh jqi@${host} "cd ~; rm -rf logs; mkdir logs; docker run --name smart${index} --net=host --cap-add NET_ADMIN smart bash /home/runscripts/smartrun.sh bftsmart.demo.microbenchmarks.ThroughputLatencyServer ${index} 10 0 0 false nosig rwd >logs/log_${index}.log 2>&1 &"
    let index=$index+1
done
