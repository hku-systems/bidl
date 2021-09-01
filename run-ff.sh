#!/bin/bash

all=9 #9
peers=5

# tput calculation interval
interval=500

if [ $1 == "performance" ]; then 
    bash create_artifact.sh fastfabric
    rm -rf logs/ff/performance
    mkdir -p logs/ff/performance
    round=0
    rm log.log
    for send_rate in 4000 8000 12000 16000 20000 24000 28000 32000; do
        i=8
        j=4
        k=40
        let round=round+1
        echo $round $send_rate
        log=round_${round}_e2e_${send_rate}.log 
        # sed -i "177c num_of_conn: $i" $HOME/fastfabric_exp/tape.yaml
        # sed -i "178c client_per_conn: $j" $HOME/fastfabric_exp/tape.yaml
        # sed -i "179c threads: $k" $HOME/fastfabric_exp/tape.yaml
        phase1=round_${round}_phase1_${send_rate}.log 
        phase2=round_${round}_phase2_${send_rate}.log 
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
        # docker exec $(docker ps | grep fabric_tape | awk '{print $1}') tape --no-e2e -n 50000 --burst 50000 --num_of_conn $i --client_per_conn $j --groups $peers --send_rate $send_rate --config config.yaml > $phase1 2>&1
        docker exec $(docker ps | grep fabric_tape | awk '{print $1}') tape --e2e -n 50000 --burst 50000 --num_of_conn $i --client_per_conn $j --orderer_client $k --groups $peers --send_rate $send_rate --config config.yaml > $phase2 2>&1
        for id in 0 1 2 3 4 5; do 
            docker service logs fabric_peer$id > logs/ff/performance/round_${round}_peer$id.log 2>&1
        done
        docker stack rm fabric
        bash runall.sh "bash clean.sh"
        # echo "latency (endorse): "  >> log.log
        # cat $phase1 | python3 latency_p1.py >> log.log
        # echo "latency (commit): "  >> log.log
        cat $phase2 | python3 latency_p2.py >> log.log
        echo latency conn=$i client=$j orderer_client=$k >> log.log
        bash process-latency.sh logs/ff/performance $round >> log.log
        echo "tps: " >> log.log
        cat $phase2 | python3 tput.py $interval VALID >> log.log
        grep "tps" $phase2 >> log.log
        # TODO latency breakdown 
        mv $phase1 logs/ff/performance/
        mv $phase2 logs/ff/performance/
        sleep 20
    done
    mv log.log logs/ff/performance/
    exit 0

elif [ $1 == "nd" ]; then 
    bash create_artifact.sh fastfabric
    rm -rf logs/ff/nondeterminism
    mkdir -p logs/ff/nondeterminism
    # set endorser group size = 2
    rm log.log
    accounts=50000
    # line=$(grep -n "endorser_groups:" $HOME/fastfabric_exp/tape.yaml | awk -F : '{print $1}')
    # sed -i "${line}c endorser_groups: 5" $HOME/fastfabric_exp/tape.yaml
    round=0
    i=8
    j=4
    k=40
    send_rate=20000
    for nondeterminism_rate in 0 0.1 0.2 0.3 0.4 0.5; do 
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
        docker exec $(docker ps | grep fabric_tape | awk '{print $1}') tape --e2e -n $accounts --orderer_client $k --num_of_conn $i --client_per_conn $j --burst 50000 --txtype create_random --groups $peers --send_rate $send_rate --ndrate $nondeterminism_rate --config config.yaml > $nondeterminism 2>&1
        sleep 2

        echo "nondeterminism_rate=$nondeterminism_rate, accounts=$accounts" >> log.log
        echo -n "tps: " >> log.log
        cat $nondeterminism | python3 tput.py $interval VALID >> log.log
        # grep "tps" $nondeterminism >> log.log
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
    bash create_artifact.sh fastfabric
    rm log.log
    rm  -rf logs/ff/contention
    mkdir -p logs/ff/contention
    hot_rate=0.01
    accounts=50000
    round=0
    i=8
    j=4
    k=40
    send_rate=20000
    for contention_rate in 0 0.1 0.2 0.3 0.4 0.5; do 
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
        docker exec $(docker ps | grep fabric_tape | awk '{print $1}') tape --e2e -n $accounts --burst 50000 --groups $peers --orderer_client $k --num_of_conn $i --client_per_conn $j --send_rate $send_rate --config config.yaml > $log 2>&1
        sleep 2
        # transfer money with contention rate = $i
        docker exec $(docker ps | grep fabric_tape | awk '{print $1}') tape --e2e -n 50000 --burst 50000 --groups $peers --orderer_client $k --num_of_conn $i --client_per_conn $j --send_rate $send_rate --hrate $hot_rate --crate $contention_rate --config config.yaml > $contention 2>&1

        echo "contention_rate=$contention_rate, accounts=$accounts, hot_rate=$hot_rate" >> log.log
        echo -n "tps: " >> log.log
        cat $contention | python3 tput.py $interval VALID >> log.log
        grep "tps" $contention >> log.log

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
elif [ $1 == "breakdown" ]; then 
    if [ $2 == "endorse" ]; then 
        bash create_artifact.sh fastfabric
        # endorse phase: simulate by decreasing the clients with constant Orgs
        rm log.log
        rm -rf logs/ff/breakdown/endorse
        mkdir -p logs/ff/breakdown/endorse
        accounts=50000
        send_rate=20000
        round=0
        for i in 25 22 20 18 15 13; do 
            for j in 4; do 
                let round=round+1
                # let j=1
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
                log=round${round}_${i}_${j}_e2e.log 
                docker exec $(docker ps | grep fabric_tape | awk '{print $1}') tape --e2e -n $accounts --burst 50000 --groups $peers --num_of_conn 8 --client_per_conn 4 --orderer_client 40 --send_rate $send_rate --config config.yaml > $log 2>&1
                sleep 5
                docker exec $(docker ps | grep fabric_tape | awk '{print $1}') tape --e2e -n $accounts --burst 50000 --groups $peers --num_of_conn $i --client_per_conn $j --orderer_client $i --config config.yaml > $log 2>&1
                for id in 0 1 2 3 4 5; do 
                    docker service logs fabric_peer$id > logs/ff/breakdown/endorse/round_${round}_peer$id.log 2>&1
                done
                docker stack rm fabric
                bash runall.sh "bash clean.sh"
                echo "round=$round client=$i*$j" >> log.log
                bash process-latency.sh logs/ff/breakdown/endorse $round endorse >> log.log
                mv $log logs/ff/breakdown/endorse/
                sleep 20
            done
        done 
        mv log.log logs/ff/breakdown/endorse/
    # consensus phase: TODO 

    elif [ $2 == "commit" ]; then 
        # commit phase: simulate by decreasing the block size with constant Orgs
        rm log.log 
        rm -rf logs/ff/breakdown/commit/
        mkdir -p logs/ff/breakdown/commit/
        accounts=50000
        round=0
        for i in 1500 1200 900 600 300; do
            sed -i "188c \        MaxMessageCount: $i" configtx.yaml
            bash create_artifact.sh fastfabric
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
            log=round${round}_${i}_e2e.log 
            docker exec $(docker ps | grep fabric_tape | awk '{print $1}') tape --e2e -n $accounts --burst 50000 --groups $peers --num_of_conn 16 --client_per_conn 16 --orderer_client 40 --config config.yaml > $log 2>&1
            for id in 0 1 2 3 4 5; do 
                docker service logs fabric_peer$id > logs/ff/breakdown/commit/round_${round}_peer$id.log 2>&1
            done
            docker stack rm fabric
            bash runall.sh "bash clean.sh"
            echo "round=$round blocksize=$i" >> log.log
            bash process-latency.sh logs/ff/breakdown/commit $round commit >> log.log
            mv $log logs/ff/breakdown/commit/
            sleep 20

        done 
        mv log.log logs/ff/breakdown/commit
        sed -i "188c \        MaxMessageCount: 1500" configtx.yaml
    fi
else 
    echo "TODO ???"
fi

# line=$(grep -n "endorser_groups:" $HOME/fastfabric_exp/tape.yaml | awk -F : '{print $1}')
# echo line: $line
# sed -i "${line}c endorser_groups: 10" $HOME/fastfabric_exp/tape.yaml
# e2e tps and latency
# contention 
# nondeterminism
