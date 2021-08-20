#!/bin/bash

bash create_artifact.sh fastfabric
all=9

if [ $1 == "performance" ]; then 
    rm -rf logs/ff/performance
    mkdir -p logs/ff/performance
    round=0
    rm log.log
    for i in 2 4 8 16; do
        for jj in 2 1; do
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
                docker stack deploy --resolve-image never --compose-file=docker-compose-fastfabric.yaml fabric
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
                docker exec $(docker ps | grep fabric_tape | awk '{print $1}') tape --no-e2e -n 50000 --burst 50000 --num_of_conn $p1i --client_per_conn $p1j --groups 5 --config config.yaml > $phase1 2>&1
                docker exec $(docker ps | grep fabric_tape | awk '{print $1}') tape --no-e2e -n 50000 --burst 50000 --num_of_conn $i --client_per_conn $j --orderer_client $k --groups 5 --config config.yaml > $phase2 2>&1
                for id in 0 1 2 3 4 5; do 
                    docker service logs fabric_peer$id > logs/ff/performance/round_${round}_peer$id.log 2>&1
                done
                docker stack rm fabric
                bash runall.sh "bash clean.sh"
                # echo "latency (endorse): "  >> log.log
                # cat $phase1 | python3 latency_p1.py >> log.log
                # echo "latency (commit): "  >> log.log
                # cat $phase2 | python3 latency_p2.py >> log.log
                echo latency conn=$i client=$j orderer_client=$k >> log.log
                bash process-latency.sh logs/ff/performance $round >> log.log
                echo "tps: " >> log.log
                cat $phase2 | python3 tput.py 500 VALID >> log.log
                # TODO latency breakdown 
                mv $phase1 logs/ff/performance/
                mv $phase2 logs/ff/performance/
                sleep 20
            done
        done
    done
    mv log.log logs/ff/performance/
    exit 0

elif [ $1 == "nd" ]; then 
    rm -rf logs/ff/nondeterminism
    mkdir -p logs/ff/nondeterminism
    # set endorser group size = 2
    rm log.log
    accounts=50000
    # line=$(grep -n "endorser_groups:" $HOME/fastfabric_exp/tape.yaml | awk -F : '{print $1}')
    # sed -i "${line}c endorser_groups: 5" $HOME/fastfabric_exp/tape.yaml
    round=0
    for nondeterminism_rate in 0.1 0.2 0.3 0.4 0.5; do 
        let round=round+1
        nondeterminism=round${round}_${nondeterminism_rate}_nondeterminism.log 
        docker stack deploy --resolve-image never --compose-file=docker-compose-fastfabric.yaml fabric
        while true; do 
            wait=$(docker service list | grep 1/1 | wc -l)
            if [ $wait == $all ]; then 
                break;
            fi 
            sleep 2
        done
        sleep 2
        docker exec $(docker ps | grep fabric_cli | awk '{print $1}') bash scripts/script.sh
        # create $accounts accounts
        docker exec $(docker ps | grep fabric_tape | awk '{print $1}') tape --e2e -n $accounts --orderer_client 20 --burst 1000 --txtype create_random --groups 2 --ndrate $nondeterminism_rate --config config.yaml > $nondeterminism 2>&1
        sleep 2

        echo "nondeterminism_rate=$nondeterminism_rate, accounts=$accounts" >> log.log
        echo -n "tps: " >> log.log
        cat $nondeterminism | python3 tput.py 500 VALID >> log.log
        for id in 0 1 2 3 4 5; do 
            docker service logs fabric_peer$id > logs/ff/nondeterminism/round${round}_peer$id.log 2>&1
        done

        docker stack rm fabric
        bash runall.sh "bash clean.sh"
        mv $nondeterminism logs/ff/nondeterminism/
        sleep 20
    done
    mv log.log logs/ff/nondeterminism/
    exit 0
elif [ $1 == "contention" ]; then 
    rm log.log
    rm  -rf logs/ff/contention
    mkdir -p logs/ff/contention
    hot_rate=0.01
    accounts=100000
    round=0
    for contention_rate in 0.1 0.2 0.3 0.4 0.5; do 
        let round=round+1
        docker stack deploy --resolve-image never --compose-file=docker-compose-fastfabric.yaml fabric
        while true; do 
            wait=$(docker service list | grep 1/1 | wc -l)
            if [ $wait == $all ]; then 
                break;
            fi 
            sleep 2
        done
        sleep 2
        docker exec $(docker ps | grep fabric_cli | awk '{print $1}') bash scripts/script.sh
        # create $accounts accounts
        log=round${round}_${contention_rate}_e2e.log 
        contention=round${round}_${contention_rate}_contention.log 
        docker exec $(docker ps | grep fabric_tape | awk '{print $1}') tape --e2e -n $accounts --burst 50000 --groups 5 --orderer_client 20 --config config.yaml > $log 2>&1
        sleep 2
        # transfer money with contention rate = $i
        docker exec $(docker ps | grep fabric_tape | awk '{print $1}') tape --e2e -n 50000 --burst 50000 --groups 5 --orderer_client 20 --hrate $hot_rate --crate $contention_rate --config config.yaml > $contention 2>&1

        echo "contention_rate=$contention_rate, accounts=$accounts, hot_rate=$hot_rate" >> log.log
        echo -n "tps: " >> log.log
        cat $contention | python3 tput.py 500 VALID >> log.log

        for id in 0 1 2 3 4 5; do 
            docker service logs fabric_peer$id > logs/ff/contention/round${round}_peer$id.log 2>&1
        done
        docker stack rm fabric
        bash runall.sh "bash clean.sh"
        mv $log logs/ff/contention/
        mv $contention logs/ff/contention/
        sleep 20
    done
    mv log.log logs/ff/contention
    exit 0
else 
    echo "???"
fi

# line=$(grep -n "endorser_groups:" $HOME/fastfabric_exp/tape.yaml | awk -F : '{print $1}')
# echo line: $line
# sed -i "${line}c endorser_groups: 10" $HOME/fastfabric_exp/tape.yaml
# e2e tps and latency
# contention 
# nondeterminism