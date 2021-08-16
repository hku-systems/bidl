#!/bin/bash

# install docker and setup network
# ...

mkdir -p $HOME/go
export GOPATH=$HOME/go

## init deploy 
bash runall.sh "docker pull hyperledger/fabric-ccenv:1.3.0; docker tag hyperledger/fabric-ccenv:latest"

## deploy fastfabric
git clone XXXfabric
git checkout 4ece3dd
cd $GOPATH/src/github.com/hyperledger/fabric
make peer-docker orderer-docker tools-docker
bash deploy.sh fastfabric

## deploy fabric
git clone XXX 
git checkout ???
cd $GOPATH/src/github.com/hyperledger/fabric
make peer-docker orderer-docker tools-docker
bash deploy.sh fabric


## deploy streamchain
echo "clone streamchain "
git clone yunpeng@10.22.1.6:~/code/streamchain-benchmarks streamchain
cd streamchain/setup
. config.sh
for host in $peers; then 
    ssh -tt $USER@$host "mkdir -p $bm_path; sudo mount -t tmpfs -o size=8G $bm_path"
done 


## tape
git clone XXXtape 
cd tape
# build 
# deploy