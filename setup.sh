#!/bin/bash

# install docker and setup network
# ...
cur=$PWD

mkdir -p $HOME/go
export GOPATH=$HOME/go

## init deploy 
# bash runall.sh "docker pull hyperledger/fabric-ccenv:1.3.0; docker tag hyperledger/fabric-ccenv:latest"

## deploy fastfabric
# mkdir -p $GOPATH/src/github.com/hyperledger
# rm -rf $GOPATH/src/github.com/hyperledger/fabric
# git clone qiji@10.22.1.8:~/go/src/github.com/hyperledger/fabric $GOPATH/src/github.com/hyperledger/fabric
# git checkout artifact
cd $GOPATH/src/github.com/hyperledger/fabric
make peer-docker orderer-docker tools-docker
bash deploy.sh fastfabric

## deploy fabric
# git clone qiji@10.22.1.8:~/go/src/github.com/hyperledger/fabric
# $GOPATH/src/github.com/hyperledger/fabric

cd $GOPATH/src/github.com/hyperledger/fabric
git checkout .
git checkout bbfaf92
make peer-docker-clean orderer-docker-clean tools-docker-clean
make peer-docker orderer-docker tools-docker
bash deploy.sh fabric


## deploy streamchain
cd $cur
echo "clone streamchain "
git clone yunpeng@10.22.1.6:~/code/streamchain-benchmarks streamchain
cd streamchain/setup
. config.sh
for host in $peers; do
    ssh -tt $USER@$host "mkdir -p $bm_path; sudo mount -t tmpfs -o size=8G $bm_path"
done 


# ## deploy hotstuff
# git clone yunpeng@10.22.1.1:~/hotstuff 
# cd hotstuff
# cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED=ON 
# make -j
#     ## Please setup related configuration following the README.md
# bash docker/deploy.sh hotstuff1.0
# 
# ## deploy SBFT
# git clone yunpeng@10.22.1.6:~/code/SBFT
# cd SBFT
# docker build -f Dockerfile_base -t sbft:base .

## tape

cd $cur
git clone qiji@10.22.1.8:~/tape
git checkout hot
cd tape
docker build -f Dockerfile -t tape .