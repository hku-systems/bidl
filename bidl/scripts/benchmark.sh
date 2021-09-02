#!/bin/bash -e

if [ "$#" -lt 2 ]; then
    echo "Usage: bash ./bidl/scripts/benchmark.sh <1. num of orgs> <2. benchmark> <3. benchmark parameters*>"
    exit 1
fi

echo "benchmarking..."
cd $normal_node_dir
if [ $2 == "performance" ]; then
    # docker run --name bidl_client --net=host --cap-add NET_ADMIN normal_node /normal_node/client --num=100000 --org=$1
    docker run --name bidl_client --net=host --cap-add NET_ADMIN normal_node /normal_node/client --num=260000 --org=$1 --order --startSeq=$3
    # docker run --name bidl_client --net=host --cap-add NET_ADMIN normal_node /normal_node/client --num=510000 --org=$1 --order --startSeq=$3
elif [ $2 == "nd" ]; then 
    docker run --name bidl_client --net=host --cap-add NET_ADMIN normal_node /normal_node/client --num=100000 --org=$1 --nd=$3
elif [ $2 == "contention" ]; then 
    docker run --name bidl_client --net=host --cap-add NET_ADMIN normal_node /normal_node/client --num=100000 --org=$1 --conflict=$3
elif [ $2 == "scalability" ]; then 
    docker run --name bidl_client --net=host --cap-add NET_ADMIN normal_node /normal_node/client --num=100000 --org=$1
elif [ $2 == "malicious" ]; then 
    docker run --name bidl_client --net=host --cap-add NET_ADMIN normal_node /normal_node/client --num=260000 --org=$1 --order --malicious --startSeq=$3
else 
    echo "Invalid argument."
    exit 1
fi
