#!/bin/bash

# install docker and setup network

# mkdir -p /proj/bidl-PG0/docker
# sudo ln -s /proj/bidl-PG0/docker /var/lib/docker

bash scp.sh presetup.sh 
bash runall.sh "bash presetup.sh"
IP=$(curl ifconfig.me)
sudo docker swarm init --advertise-addr $IP #TODO 
sudo docker swarm join-token manager > docker.info
cmd=$(grep docker docker.info)
bash runall.sh "$cmd"
sudo docker network create --driver overlay --attachable HLF

rm -rf logs
mkdir -p /proj/bidl-PG0/logs
ln -s /proj/bidl-PG0/logs logs

sudo apt install python3-pip -y
pip3 install numpy matplotlib


## init deploy 
bash runall.sh "docker pull hyperledger/fabric-ccenv:1.3.0; docker tag hyperledger/fabric-ccenv:1.3.0 hyperledger/fabric-ccenv:latest"

cur=$PWD

## deploy fabric
newgrp docker << END
source ~/.bashrc
rm -rf $GOPATH/src/github.com/hyperledger/fabric
mkdir -p fabric $GOPATH/src/github.com/hyperledger
cp -r fabric $GOPATH/src/github.com/hyperledger/fabric
cd $GOPATH/src/github.com/hyperledger/fabric
go mod init
go mod vendor
make peer-docker-clear orderer-docker-clear tools-docker-clear
make peer-docker orderer-docker tools-docker
cd $cur
exit 0
END
bash deploy.sh fabric-v1.0

## deploy fastfabric
newgrp docker << END
source ~/.bashrc
mkdir -p $GOPATH/src/github.com/hyperledger
rm -rf $GOPATH/src/github.com/hyperledger/fabric
mkdir -p fabric $GOPATH/src/github.com/hyperledger
cp -r fastfabric $GOPATH/src/github.com/hyperledger/fabric
cd $GOPATH/src/github.com/hyperledger/fabric
go mod init
go mod vendor
make peer-docker-clear orderer-docker-clear tools-docker-clear
make peer-docker orderer-docker tools-docker
cd $cur
exit 0
END
bash deploy.sh fastfabric-v1.0

## deploy streamchain
cd streamchain/setup
bash reconfigure.sh 
. config.sh
for host in $peers; do
    ssh -tt $USER@$host "mkdir -p $bm_path; sudo mount -t tmpfs -o size=8G $bm_path"
done 

## tape
cd $cur
cd tape
sudo docker build -f Dockerfile -t tape .
newgrp docker
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
