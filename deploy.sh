#!/bin/bash

. config.sh

tag=latest

if [ $# -eq 1 ]; then 
    tag=$1
fi

echo "image tag: $tag"

docker tag hyperledger/fabric-peer:latest hyperledger/fabric-peer:$tag
docker tag hyperledger/fabric-orderer:latest hyperledger/fabric-orderer:$tag
docker tag hyperledger/fabric-tools:latest hyperledger/fabric-tools:$tag

docker save -o hlf_peer_${tag}.tar hyperledger/fabric-peer:$tag
docker save -o hlf_orderer_${tag}.tar hyperledger/fabric-orderer:$tag
# docker save -o hlf_tools_${tag}.tar hyperledger/fabric-tools:$tag
# docker save -o hlf_ccenv.tar hyperledger/fabric-ccenv:$tag

for host in ${hosts[@]}; do
    echo "###  $host  ###"
    scp clean.sh $USER@$host:~/
    scp hlf_orderer_${tag}.tar $USER@$host:~/
    scp hlf_peer_${tag}.tar $USER@$host:~/
    # scp hlf_tools_${tag}.tar $USER@$host:~/
done

rm hlf_orderer_${tag}.tar hlf_peer_${tag}.tar # hlf_tools_${tag}.tar

# deploy to all servers
# bash runall.sh "docker load --input hlf_orderer_${tag}.tar; docker load --input hlf_peer_${tag}.tar; docker load --input hlf_tools_${tag}.tar"
bash runall.sh "docker load --input hlf_orderer_${tag}.tar; docker load --input hlf_peer_${tag}.tar"

