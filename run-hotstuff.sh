#!/bin/bash 
cd hotstuff
bash docker/run.sh hotstuff1.0
docker exec c0 bash run.sh setup  
# set payload size in scripts/doploy/group_vars/all.yml in container c0
docker exec c0 bash gen_all.sh 
docker exec c0 bash run.sh new run1
docker exec c0 bash run_cli.sh new run1_cli
sleep 30s
docker exec c0 bash run_cli.sh stop run1_cli
docker exec c0 bash run_cli.sh fetch run1_cli 
docker exec c0 cat run1_cli/remote/*/stderr | python3 ../thr_hist.py --plot
bash docker/kill_docker.sh 
