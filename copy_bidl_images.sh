#!/bin/bash
set -eu

echo "Deploy BIDL's docker image to all servers..."
base_dir=./bidl
for i in 1 2 3 4 5 6 9 19; do
    host=10.22.1.$i
    echo $host
    scp $base_dir/smart.tar ${USER}@${host}:~/
    scp $base_dir/normal_node.tar ${USER}@${host}:~/
    scp $base_dir/sequencer.tar ${USER}@${host}:~/
    ssh ${USER}@${host} 'docker rm $(docker ps -aq --filter name="smart"); docker rmi smart; docker load --input smart.tar;
    docker rm $(docker ps -aq --filter name="normal_node"); docker rmi normal_node; docker load --input normal_node.tar;
    docker rm $(docker ps -aq --filter name="sequencer"); docker rmi sequencer; docker load --input sequencer.tar'
done
