#!/bin/bash 

echo "run fastfabric "
docker stack rm fabric
bash runall.sh "bash clean.sh"
bash create_artifact.sh fastfabric
docker stack deploy --compose-file=docker-compose-fastfabric.yaml fabric
sleep 5
cli=$(docker ps | grep fabric_cli | awk '{print $1}')
while [ ! $cli ]; do 
    echo "wait for cli contianer "
    sleep 5
    let cli=$(docker ps | grep fabric_cli | awk '{print $1}')
done
echo $cli
# setup the channel and deploy the smart conatract
timeout 120 docker exec $cli bash scripts/script.sh 
if [ $? -ne 0 ]; then 
    docker stack rm fabric 
    bash runall.sh "bash clean.sh"
    echo "something failed, rerun this script"
    bash example.sh
    exit 0
fi
sleep 2
# create 5000 accounts
tape=$(docker ps | grep fabric_tape | awk '{print $1}')
while [ ! $tape ]; do 
    echo "wait for tape contianer "
    sleep 5
    let tape=$(docker ps | grep fabric_tape | awk '{print $1}')
done
echo $tape
docker exec $tape tape --e2e -n 50000 --burst 50000 --txtype create --num_of_conn 8 --client_per_conn 4 --send_rate 25000 --orderer_client 40 --groups 5 --config config.yaml > create.log 2>&1
sleep 5
# send 5000 payment transactions
docker exec $tape tape --e2e -n 50000 --burst 50000 --txtype transfer --num_of_conn 8 --client_per_conn 4 --send_rate 25000 --orderer_client 40 --groups 5 --config config.yaml > transfer.log 2>&1
# stop the network 
docker stack rm fabric 
# remove all containers
bash runall.sh "bash clean.sh"
# report the result
echo "#################################################"
echo "create 50000 acocunts"
grep "tps" create.log 
echo "start 50000 payment transactions"
grep "tps" transfer.log 
echo -n "conflict: "
cat transfer.log | python3 conflict.py
echo "#################################################"