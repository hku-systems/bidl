#!/bin/bash 

cd SBFT 
# rm -rf ../logs/sbft
# mkdir -p ../logs/sbft

for n in 4 13 25 31 49 61 76 91; do 
    docker run -it --rm -d --name s0 -v $PWD:/home/SBFT sbft:base 
    docker exec s0 bash build.sh 
    f=$(echo "($n-1)/3" | bc)
    check=$(echo "$f*3+1" | bc)
    if [ $check -ne $n ]; then 
        echo "please make sure n equals to 3f+1"
        exit 1
    fi
    docker exec s0 bash evaluation/run.sh $n $f 200 500 # n,f,c,ops

    echo "$n $f 200 500" >> ../logs/sbft/log.log
    cat log_client* | python3 evaluation/tput.py >> ../logs/sbft/log.log 2>&1 

    docker exec s0 bash evaluation/stop.sh 
    docker stop s0

    sleep 10
done 