#!/bin/bash

rm -rf logs/fabric
mkdir -p logs/fabric

docker stack rm fabric 
bash runall.sh "bash clean.sh"
# e2e tps and latency
bash create_artifact.sh fabric
all=9
round=0
rm log.log
send_rates=(2000 4000 6000 8000 10000 16000)
len=${#send_rates[@]}
curi=0
while [ $curi -lt $len ]; do 
# for send_rate in 1000 2000 3000 4000 5000 8000 16000; do
    send_rate=${send_rates[$curi]}
    echo "send_rate:$send_rate"
    i=8
    j=4
    k=40
    echo $i $j $round
    log=round_${round}_e2e_${send_rate}.log 
    phase1=round_${round}_phase1_${send_rate}.log 
    phase2=round_${round}_phase2_${send_rate}.log 
    docker stack deploy --resolve-image never --compose-file=docker-compose-fabric.yaml fabric
    cnt=0
    while true; do 
        wait=$(docker service list | grep 1/1 | wc -l)
        if [ $wait == $all ]; then 
            break;
        fi 
        sleep 2
        let cnt=cnt+1
        echo "waiting... $cnt"
        if [ $cnt -gt 30 ]; then 
            let cnt=-1
            break 
        fi
    done
    if [ $cnt -eq -1 ]; then 
        docker stack rm fabric 
        bash runall.sh "bash clean.sh"
        echo "something failed, rerun round$round "
        sleep 10
        continue
    fi 
    sleep 2
    cli=$(docker ps | grep fabric_cli | awk '{print $1}')
    while [ ! $cli ]; do 
        echo "fatal error, please rerun this script:)"
        exit 1
    done
    echo $cli
    timeout 120 docker exec $cli bash scripts/script.sh
    if [ $? -ne 0 ]; then 
        docker stack rm fabric 
        bash runall.sh "bash clean.sh"
        echo "something failed, rerun round$round "
        sleep 10
        continue
    fi
    # create 50000 accounts
    # docker exec $(docker ps | grep fabric_tape | awk '{print $1}') tape --no-e2e -n 50000 --burst 50 --num_of_conn $p1i --client_per_conn $p1j --groups 5 --config config.yaml > $phase1 2>&1
    docker exec $(docker ps | grep fabric_tape | awk '{print $1}') tape --e2e -n 50000 --burst 50000 --num_of_conn $i --client_per_conn $j --send_rate $send_rate --orderer_client $k --groups 5 --config config.yaml > $log 2>&1
    for id in 0 1 2 3 4 5; do 
        docker service logs fabric_peer$id > logs/fabric/round_${round}_peer$id.log 2>&1
    done
    docker service logs fabric_orderer > logs/fabric/round_${round}_orderer.log 2>&1
    docker stack rm fabric
    bash runall.sh "bash clean.sh"
    # echo "latency (endorse): "  >> log.log
    # cat $phase1 | python3 latency_p1.py >> log.log
    # echo "latency (commit): "  >> log.log
    cat $log | python3 latency_p2.py >> log.log
    echo latency conn=$i client=$j orderer_client=$k >> log.log
    bash process-latency.sh logs/fabric/ $round >> log.log
    echo "tps: " >> log.log
    cat $log | python3 tput.py 500 VALID >> log.log
    # TODO latency breakdown 
    # mv $phase1 logs/fabric/
    mv $log logs/fabric/
    let curi=curi+1
    let round=round+1
    sleep 10
done
mv log.log logs/fabric
