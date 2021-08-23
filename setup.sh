#!/bin/bash

# install docker and setup network
# ...
cur=$PWD

mkdir -p $HOME/go
export GOPATH=$HOME/go

## init deploy 
# bash runall.sh "docker pull hyperledger/fabric-ccenv:1.3.0; docker tag hyperledger/fabric-ccenv:latest"

## deploy fastfabric
mkdir -p $GOPATH/src/github.com/hyperledger
rm -rf $GOPATH/src/github.com/hyperledger/fabric
cp -r fastfabric $GOPATH/src/github.com/hyperledger/fabric
cd $GOPATH/src/github.com/hyperledger/fabric
make peer-docker orderer-docker tools-docker
cd $cur
bash deploy.sh fastfabricv1.0

## deploy fabric

cd $GOPATH/src/github.com/hyperledger/fabric
rm -rf $GOPATH/src/github.com/hyperledger/fabric
cp -r fabric $GOPATH/src/github.com/hyperledger/fabric
cd $GOPATH/src/github.com/hyperledger/fabric
make peer-docker orderer-docker tools-docker
cd $cur
bash deploy.sh fabric


## deploy streamchain
cd streamchain/setup
bash reconfigure.sh 
cd $cur
# . config.sh
# for host in $peers; do
#     ssh -tt $USER@$host "mkdir -p $bm_path; sudo mount -t tmpfs -o size=8G $bm_path"
# done 

## tape
cd $cur
git clone qiji@10.22.1.8:~/tape
git checkout hot
cd tape
docker build -f Dockerfile -t tape .

exit 0

## deploy hotstuff
cd hotstuff
# Please setup related configuration following the README.md
bash docker/deploy.sh hotstuff1.0
cd $cur

## deploy SBFT
# scp -r yunpeng@10.22.1.6:~/code/SBFT SBFT
cd SBFT
docker build -f Dockerfile_base -t sbft:base .
cd $cur
