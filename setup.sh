#!/bin/bash

# install docker and setup network
bash scp.sh presetup.sh 
bash runall.sh "bash presetup.sh"
IP=$(ifconfig eno1 | grep "inet " | awk '{print $2}')
sudo docker swarm init --advertise-addr $IP #TODO 
sudo docker swarm join-token manager > docker.info
cmd=$(grep docker docker.info)
bash runall.sh "$cmd"
sudo docker network create --driver overlay --attachable HLF

# install golang
wget https://golang.org/dl/go1.17.linux-amd64.tar.gz
sudo rm -rf /usr/local/go && sudo tar -C /usr/local -xzf go1.17.linux-amd64.tar.gz

sudo apt install python3-pip
pip3 install numpy matplotlib

cur=$PWD

mkdir -p $HOME/go
export GOPATH=$HOME/go

## init deploy 
bash runall.sh "docker pull hyperledger/fabric-ccenv:1.3.0; docker tag hyperledger/fabric-ccenv:1.3.0 hyperledger/fabric-ccenv:latest"

## deploy fastfabric
mkdir -p $GOPATH/src/github.com/hyperledger
rm -rf $GOPATH/src/github.com/hyperledger/fabric
cp -r fastfabric $GOPATH/src/github.com/hyperledger/fabric
cd $GOPATH/src/github.com/hyperledger/fabric
make peer-docker-clear orderer-docker-clear tools-docker-clear
make peer-docker orderer-docker tools-docker
cd $cur
bash deploy.sh fastfabricv1.0

## deploy fabric

rm -rf $GOPATH/src/github.com/hyperledger/fabric
cp -r fabric $GOPATH/src/github.com/hyperledger/fabric
cd $GOPATH/src/github.com/hyperledger/fabric
make peer-docker-clear orderer-docker-clear tools-docker-clear
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
