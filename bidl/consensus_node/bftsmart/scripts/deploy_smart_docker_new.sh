#!/bin/bash -e
if [ "$#" -ne 1 ]; then
    echo "Usage: ... ./scripts/start_smart_docker.sh <num of nodes>"
    exit 1
fi

host_num=`cat ./scripts/servers|wc -l`
nodes_per_host=$[$[$1-1]/$[${host_num}-1]]
echo "Nodes per host is $nodes_per_host"

echo "######################################################"
echo "Deploy $1 nodes on $host_num servers"
echo "######################################################"

echo "Generating hosts.config"
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

echo "Copy system_$1.config to ./config/system.config"
cp ./scripts/configs/system_$1.config ./config/system.config

echo "Compiling..."
ant

echo "Building image..."
rm -f config/currentView
rm -f smart.tar
rm -rf ./logs
docker image build -t smart .
docker save -o smart.tar smart

# stop running containers on all servers, must stop before deploy, or the old image cannot be deleted
./scripts/kill_smart_docker.sh

# deploy to all servers
./scripts/copy_smart_docker.sh


# echo 'Starting '$(cat ./scripts/servers|wc -l)' nodes...'

echo 'Starting '$(cat ./config/hosts.config|wc -l)' nodes...'
while read line; do
    node=$(echo $line | awk '{print $1}')
    host=$(echo $line | awk '{print $2}')
    echo ${node} ${host}
    ssh -n jqi@${host} "cd ~; rm -rf logs; mkdir logs; docker run --name smart${node} --net=host --cap-add NET_ADMIN smart bash /home/runscripts/smartrun.sh bftsmart.demo.microbenchmarks.ThroughputLatencyServer ${node} 10 0 0 false nosig rwd >logs/log_${node}.log 2>&1 &"
done <./config/hosts.config

# echo 'Starting frontends...'
# let index=0
# for host in `cat ./config/hosts.config|awk '{print $2}'`; do
#     if [ $index -eq 0 ]; 
#     then
#         ssh jqi@${host} "docker run --net=host smart bash /home/runscripts/smartrun.sh bftsmart.demo.bidl.BidlFrontendSequencer 231.0.0.0 7777 true > logs/frontend.log 2>&1 &"
#     else
#         ssh jqi@${host} "docker run --net=host smart bash /home/runscripts/smartrun.sh bftsmart.demo.bidl.BidlFrontendSequencer 231.0.0.0 7777 false > logs/frontend.log 2>&1 &"
#     fi
#     let index=$index+1
# done