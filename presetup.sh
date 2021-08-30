#!/bin/bash 

# install docker
sudo apt update
sudo apt install docker.io -y
sudo groupadd docker 
sudo usermod -aG docker $USER

# install golang
wget https://golang.org/dl/go1.17.linux-amd64.tar.gz
sudo rm -rf /usr/local/go && sudo tar -C /usr/local -xzf go1.17.linux-amd64.tar.gz
echo "export PATH=\$PATH:/usr/local/go/bin" >> ~/.bashrc

# set GOPATH
mkdir -p $HOME/go
echo "export GOPATH=$HOME/go" >> ~/.bashrc