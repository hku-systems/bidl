#!/bin/bash 

echo "run fastfabric "
docker stack rm fabric
bash runall.sh "bash clean.sh"
bash create_artifact.sh fastfabric
docker stack deploy --compose-file=docker-compose-fastfabric.yaml fabric
sleep 2
# setup the channel and deploy the smart conatract
docker exec $(docker ps | grep fabric_cli | awk '{print $1}') bash scripts/script.sh 
sleep 2
# create 5000 accounts
docker exec $(docker ps | grep fabric_tape | awk '{print $1}') tape --e2e -n 50000 --burst 50000 --txtype create --num_of_conn 8 --client_per_conn 4 --send_rate 25000 --orderer_client 40 --groups 5 --config config.yaml > create.log 2>&1
sleep 5
# send 5000 payment transactions
docker exec $(docker ps | grep fabric_tape | awk '{print $1}') tape --e2e -n 50000 --burst 50000 --txtype transfer --num_of_conn 8 --client_per_conn 4 --send_rate 25000 --orderer_client 40 --groups 5 --config config.yaml > transfer.log 2>&1
# stop the network 
docker stack rm fabric 
# remove all containers
bash runall.sh "bash clean.sh"
# report the result
echo "#################################################"
echo "create 5000 acocunts"
grep "tps" create.log 
echo "start 5000 payment transactions"
grep "tps" transfer.log 
cat transfer.log | python3 conflict.py
echo "#################################################"
