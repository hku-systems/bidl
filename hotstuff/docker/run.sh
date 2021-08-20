#!/bin/bash

# !/bin/bash
echo "Stoping running containers"

# user=$(cat docker/config.sh | awk -F '#' '{print $1}' | grep "user"  | awk -F
# '=' '{print $2}' | awk '$1=$1')
. docker/config.sh

hosts=($(cat ./docker/servers))
len=${#hosts[*]}

tag=ori 
if [ $# -eq 1 ]; then 
    tag=$1
fi

bash docker/kill_docker.sh

# generate hotstuff.conf
# TODO

# run

# for host in ${hosts[*]}; do
#     scp hotstuff.conf ${user}@${host}:~/logs_hot/hotstuff.conf 
# done

node=-1
ip=50
for host in ${hosts[*]}; do
    let node=$node+1
    let ip=$ip+1
    echo $host $node
    # ssh ${user}@${host} " cd; rm -rf logs_hot/log_${node}.log ; mkdir -p logs_hot; docker run --name hotstuff${node} -v /home/${user}/logs_hot/hotstuff.conf:/home/libhotstuff/hotstuff.conf --net=host --cap-add NET_ADMIN hotstuff:$tag ./examples/hotstuff-app --conf ./hotstuff-sec${node}.conf > logs_hot/log_${node} 2>&1 & "
    ssh ${user}@${host} " cd; rm -rf logs_hot/log_${node}.log ; mkdir -p logs_hot; docker run -it -d --ip 10.0.2.$ip --name hotstuff${node} --net=HLF --cap-add NET_ADMIN hotstuff:$tag "

done

docker run -it --rm -d --ip 10.0.2.30 --net HLF --name c0 -v $PWD:/home/libhotstuff hotstuff:$tag
