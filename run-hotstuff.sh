#!/bin/bash 
cd hotstuff
bash docker/run.sh hotstuff-v1.0
docker exec c0 bash run.sh setup  
# set payload size in scripts/doploy/group_vars/all.yml in container c0
docker exec c0 bash gen_all.sh 
docker exec c0 bash run.sh new run1
docker exec c0 bash run_cli.sh new run1_cli
sleep 30s
docker exec c0 bash run_cli.sh stop run1_cli
docker exec c0 bash run_cli.sh fetch run1_cli 
cat scripts/deploy/run1_cli/remote/*/log/stderr | python3 scripts/thr_hist.py --plot > log.log
rm -rf ../logs/hotstuff
mkdir -p ../logs/hotstuff
mv log.log ../logs/hotstuff/
bash docker/kill_docker.sh 
