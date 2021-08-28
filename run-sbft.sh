#!/bin/bash 

cd SBFT 
docker run -it --rm -d --name s0 -v $PWD:/home/SBFT sbft:base 

docker exec s0 bash build.sh 

docker exec s0 bash evaluation/run.sh 200 500

cat log_client* | python3 evaluation/tput.py > log.log 2>&1 

docker exec s0 bash evaluation/stop.sh 

docker stop s0

rm -rf ../logs/sbft
mkdir -p ../logs/sbft
mv log.log ../logs/sbft/