#!/bin/bash

round=0
rm log.log
python3 generate.py $1
for i in 2 4 8 16; do
    for jj in 2 1; do
        j=$(echo "$i/$jj"|bc)
        for k in 10; do
            let round=round+1
            log=round_${round}_e2e_${i}_${j}.log 
            sed -i "177c num_of_conn: $i" tape.yaml
            sed -i "178c client_per_conn: $j" tape.yaml
            sed -i "179c threads: $k" tape.yaml
            docker stack deploy --compose-file docker-compose.yaml fabric
            docker exec $(docker ps | grep fabric_cli | awk '{print $1}')  bash scripts/script.sh 
            sleep 2
            docker exec $(docker ps | grep fabric_tape | awk '{print $1}') rm ACCOUNTS ENDORSEMENT  
            docker exec $(docker ps | grep fabric_tape | awk '{print $1}') tape --e2e -n 50000 --burst 20000 --config config.yaml > ${log} 2>&1
            for id in 0 1 2 3 4 5; do 
                docker service logs fabric_peer$id > round_${round}_peer$id.log 2>&1
            done
            docker stack rm fabric
            echo $log >> log.log
            cat $log | python3 latency.py >> log.log
            grep tps $log >> log.log
            echo ""
            sleep 20
        done 
    done
done

