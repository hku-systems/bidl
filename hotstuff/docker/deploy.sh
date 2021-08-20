#!/bin/bash


tag=ori

if [ $# -eq 1 ]; then 
    tag=$1
fi

rm -f hotstuff.tar
echo "image tag: $tag"
echo "Building image"
rm log*

ok=$(docker images | grep hotstuff | grep base | wc -l)
if [ $ok -ne 1 ]; then 
    echo "build hotstuff:base image"
    docker build -f ./Dockerfile_base -t hotstuff:base .
fi

docker build -f ./Dockerfile -t hotstuff:$tag .
docker save -o hotstuff.tar hotstuff:$tag

# stop running containers on all servers, must stop before deploy, or the old image cannot be deleted
./docker/kill_docker.sh

# deploy to all servers
./docker/copy_hotstuff_docker.sh $tag



