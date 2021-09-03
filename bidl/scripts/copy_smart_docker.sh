#!/bin/bash
echo "Deploy docker image to all servers..."
script_dir=$(cd "$(dirname "$0")";pwd)
source $script_dir/env.sh
echo $base_dir
for host in `cat $base_dir/scripts/servers`; do
    echo $host
    scp $base_dir/smart.tar ${USER}@${host}:~/
    scp $base_dir/normal_node.tar ${USER}@${host}:~/
    scp $base_dir/sequencer.tar ${USER}@${host}:~/
    ssh ${USER}@${host} 'docker rm $(docker ps -aq --filter name="smart"); docker rmi smart; docker load --input smart.tar;
    docker rm $(docker ps -aq --filter name="normal_node"); docker rmi normal_node; docker load --input normal_node.tar;
    docker rm $(docker ps -aq --filter name="sequencer"); docker rmi sequencer; docker load --input sequencer.tar'
done
