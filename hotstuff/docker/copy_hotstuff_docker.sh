#!/bin/bash
echo "Deploy docker image to all servers..."

. docker/config.sh

tag=$1

for host in `cat ./docker/servers`; do
    echo $host
    scp hotstuff.tar ${user}@${host}:~/
    # ssh ${user}@${host} 'docker rm $(docker ps -aq --filter name="hotstuff"); docker rmi hotstuff:'${tag}'; docker load --input hotstuff.tar'
    ssh ${user}@${host} 'docker load --input hotstuff.tar'
done