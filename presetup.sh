#!/bin/bash 

sudo apt update
sudo apt install docker.io -y
sudo groupadd docker 
sudo usermod -aG docker $USER
