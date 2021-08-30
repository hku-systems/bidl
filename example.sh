#!/bin/bash 

echo "run fabric "
bash create_artifact.sh fabric
docker stack deploy --compose-file=docker-compose-fabric.yaml fabric
sleep 2
docker exec $(docker ps | grep fabric_cli | awk '{print $1}') bash scripts/script.sh 
docker exec $(docker ps | grep fabric_tape | awk '{print $1}') tape --e2e -n 5000 --burst 50000 --num_of_conn 2 --client_per_conn 2 --send_rate 500 --orderer_client 16 --groups 5 --config config.yaml > log.log 2>&1
docker stack rm fabric 
bash runall.sh "bash clean.sh"
grep "tps" log.log 
rm log.log

sleep 10


echo "run fastfabric "
bash create_artifact.sh fastfabric
docker stack deploy --compose-file=docker-compose-fastfabric.yaml fabric
sleep 2
docker exec $(docker ps | grep fabric_cli | awk '{print $1}') bash scripts/script.sh 
docker exec $(docker ps | grep fabric_tape | awk '{print $1}') tape --e2e -n 5000 --burst 50000 --num_of_conn 2 --client_per_conn 2 --send_rate 500 --orderer_client 16 --groups 5 --config config.yaml > log.log 2>&1
docker stack rm fabric 
bash runall.sh "bash clean.sh"
grep "tps" log.log 
rm log.log
