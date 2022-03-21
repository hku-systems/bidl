#!/bin/bash

# 1. 

echo "Stoping running containers"

# user=$(cat docker/config.sh | awk -F '#' '{print $1}' | grep "user"  | awk -F
# '=' '{print $2}' | awk '$1=$1')
. docker/config.sh # load user and password

hosts=($(cat ./docker/servers))
len=${#hosts[*]}

# bash docker/kill_docker.sh

# generate hotstuff.conf
# TODO

# run

# for host in ${hosts[*]}; do
#     scp hotstuff.conf ${user}@${host}:~/logs_hot/hotstuff.conf 
# done

node=-1
tag=$1
ip=$2

# Ensure c0 container is running
isOn=$(docker images hotsuff:${tag} | wc -l)
if [ $isOn -ne 1 ]; then 
    #docker run -it --rm -d --ip 10.0.2.30 --net HLF --name c0 -v $PWD:/home/libhotstuff hotstuff:$tag
    docker run -it --rm -d --net=host --name c0 -v $PWD:/home/libhotstuff hotstuff:$tag
fi

for host in ${hosts[*]}; do
    let node=$node+1
    echo $host $node
    # ssh ${user}@${host} " cd; rm -rf logs_hot/log_${node}.log ; mkdir -p logs_hot; docker run --name hotstuff${node} -v /home/${user}/logs_hot/hotstuff.conf:/home/libhotstuff/hotstuff.conf --net=host --cap-add NET_ADMIN hotstuff:$tag ./examples/hotstuff-app --conf ./hotstuff-sec${node}.conf > logs_hot/log_${node} 2>&1 & "
    # ssh ${user}@${host} "rm -rf logs_hot/log_${node}.log ; mkdir -p logs_hot; docker run -it -d --ip 10.0.2.$ip --name hotstuff${node} --net=HLF --cap-add NET_ADMIN hotstuff:$tag"
    ssh ${user}@${host} "rm -rf logs_hot/log_${node}.log ; mkdir -p logs_hot; docker run -it -d --name hotstuff${node} --net=host --cap-add NET_ADMIN hotstuff:$tag"
    let ip=$ip+1
done

