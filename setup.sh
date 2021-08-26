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

# install golang
wget https://golang.org/dl/go1.17.linux-amd64.tar.gz
sudo rm -rf /usr/local/go && sudo tar -C /usr/local -xzf go1.17.linux-amd64.tar.gz

sudo apt install python3-pip -y
pip3 install numpy matplotlib

mkdir -p $HOME/go
echo "export PATH=\$PATH:/usr/local/go/bin" >> ~/.bashrc
echo "export GOPATH=$HOME/go" >> ~/.bashrc
source ~/.bashrc

cur=$PWD


## init deploy 
bash runall.sh "docker pull hyperledger/fabric-ccenv:1.3.0; docker tag hyperledger/fabric-ccenv:1.3.0 hyperledger/fabric-ccenv:latest"

## deploy fastfabric
mkdir -p $GOPATH/src/github.com/hyperledger
rm -rf $GOPATH/src/github.com/hyperledger/fabric
cp -r fastfabric $GOPATH/src/github.com/hyperledger/fabric
cd $GOPATH/src/github.com/hyperledger/fabric
go mod init
go mod vendor
make peer-docker-clear orderer-docker-clear tools-docker-clear
make peer-docker orderer-docker tools-docker
cd $cur
bash deploy.sh fastfabric-v1.0

echo "fastfabric done"

## deploy fabric

rm -rf $GOPATH/src/github.com/hyperledger/fabric
cp -r fabric $GOPATH/src/github.com/hyperledger/fabric
cd $GOPATH/src/github.com/hyperledger/fabric
go mod init
go mod vendor
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
