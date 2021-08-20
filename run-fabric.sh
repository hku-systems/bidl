#!/bin/bash

rm -rf logs/fabric
mkdir -p logs/fabric

# e2e tps and latency
bash create_artifact.sh fabric
all=9
round=0
rm log.log
for i in 1 2 3 4; do
    for jj in 1; do
        j=$(echo "$i/$jj"|bc)
        for k in 10; do
            p1i=$i
            p1j=$j
            k=$(echo "$i*1"|bc)
            let round=round+1
            echo $i $j $round
            log=round_${round}_e2e_${i}_${j}.log 
            # sed -i "177c num_of_conn: $i" $HOME/fastfabric_exp/tape.yaml
            # sed -i "178c client_per_conn: $j" $HOME/fastfabric_exp/tape.yaml
            # sed -i "179c threads: $k" $HOME/fastfabric_exp/tape.yaml
            phase1=round_${round}_phase1_${p1i}_${p1j}.log 
            phase2=round_${round}_phase2_${i}_${j}.log 
            docker stack deploy --resolve-image never --compose-file=docker-compose-fabric.yaml fabric
            while true; do 
                wait=$(docker service list | grep 1/1 | wc -l)
                if [ $wait == $all ]; then 
                    break;
                fi 
                sleep 2
            done
            sleep 2
            docker exec $(docker ps | grep fabric_cli | awk '{print $1}') bash scripts/script.sh
            # create 50000 accounts
            docker exec $(docker ps | grep fabric_tape | awk '{print $1}') tape --no-e2e -n 50000 --burst 50 --num_of_conn $p1i --client_per_conn $p1j --groups 5 --config config.yaml > $phase1 2>&1
            docker exec $(docker ps | grep fabric_tape | awk '{print $1}') tape --no-e2e -n 50000 --burst 50 --num_of_conn $i --client_per_conn $j --orderer_client $k --groups 5 --config config.yaml > $phase2 2>&1
            for id in 0 1 2 3 4 5; do 
                docker service logs fabric_peer$id > logs/fabric/round_${round}_peer$id.log 2>&1
            done
            docker stack rm fabric
            bash runall.sh "bash clean.sh"
            # echo "latency (endorse): "  >> log.log
            # cat $phase1 | python3 latency_p1.py >> log.log
            # echo "latency (commit): "  >> log.log
            # cat $phase2 | python3 latency_p2.py >> log.log
            echo latency conn=$i client=$j orderer_client=$k >> log.log
            bash process-latency.sh logs/fabric/ $round >> log.log
            echo "tps: " >> log.log
            cat $phase2 | python3 tput.py 500 VALID >> log.log
            # TODO latency breakdown 
            mv $phase1 logs/fabric/
            mv $phase2 logs/fabric/
            sleep 20
        done 
    done
done
mv log.log logs/fabric
