#!/bin/bash
echo "Deploy docker image to all servers..."
for host in `cat $base_dir/scripts/servers`; do
    echo $host
    scp $base_dir/smart.tar jqi@${host}:~/
    scp $base_dir/normal_node.tar jqi@${host}:~/
    ssh jqi@${host} 'docker rm $(docker ps -aq --filter name="smart"); docker rmi smart; docker load --input smart.tar;
    docker rm $(docker ps -aq --filter name="normal_node"); docker rmi normal_node; docker load --input normal_node.tar'
done