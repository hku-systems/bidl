#!/bin/bash 
cd hotstuff
# rm -rf ../logs/hotstuff
mkdir -p ../logs/hotstuff

for node in 31; do
    # set node IP
    echo "10.22.1.7" > docker/servers
    echo "10.22.1.9" >> docker/servers
    cnt=0
    ip=2
    while [ $cnt -lt $node ]; do 
        let cnt=cnt+1
        echo "10.22.1.$ip" >> docker/servers
        let ip=ip+1
        if [ $ip -eq 6 ]; then 
            let ip=2
        fi
    done

    # start containers: from  10.0.2.51
    bash docker/run.sh hotstuff-v1.0

    # set payload size in scripts/doploy/group_vars/all.yml in container c0
    echo "10.0.2.53 10.0.2.53" > scripts/deploy/replicas.txt
    ip=54
    for i in `seq 2 $node`; do 
        echo "10.0.2.$ip 10.0.2.$ip" >> scripts/deploy/replicas.txt
        let ip=ip+1
    done

    docker exec c0 bash gen_all.sh 
    docker exec c0 bash run.sh setup  
    docker exec c0 bash run.sh new run1
    docker exec c0 bash run_cli.sh new run1_cli
    sleep 30s
    docker exec c0 bash run_cli.sh stop run1_cli
    docker exec c0 bash run_cli.sh fetch run1_cli 
    cat scripts/deploy/run1_cli/remote/*/log/stderr | python3 scripts/thr_hist.py --plot >> ../logs/hotstuff/log_${node}.log
    echo yunpeng | sudo -S rm -rf scripts/deploy/run1 scripts/deploy/run1_cli
    bash docker/kill_docker.sh 
    sleep 10
done