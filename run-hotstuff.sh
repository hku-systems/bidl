#!/bin/bash 
cd hotstuff
# rm -rf ../logs/hotstuff
mkdir -p ../logs/hotstuff

for node in 7 16 31; do
    # set node IP
    # echo "10.22.1.7" > docker/servers
    echo "10.22.1.9" > docker/servers
    cnt=0
    ip=2
    while [ $cnt -lt $node ]; do 
        let cnt=cnt+1
        echo "10.22.1.$ip" >> docker/servers
        let ip=ip+1
        if [ $ip -eq 8 ]; then 
            let ip=2
        fi
    done

    ip=70
    # start containers: from  10.0.2.$ip
    bash docker/run.sh hotstuff-v1.0 $ip

    # set payload size in scripts/doploy/group_vars/all.yml in container c0
    echo "10.0.2.70" > scripts/deploy/clients.txt
    echo "10.0.2.71 10.0.2.71" > scripts/deploy/replicas.txt
    ip=72
    for i in `seq 2 $node`; do 
        echo "10.0.2.$ip 10.0.2.$ip" >> scripts/deploy/replicas.txt
        let ip=ip+1
    done

    docker exec c0 bash gen_all.sh 
    docker exec c0 bash run.sh setup  
    docker exec c0 bash run.sh new run$node
    docker exec c0 bash run_cli.sh new run${node}_cli
    sleep 30s
    docker exec c0 bash run_cli.sh stop run${node}_cli
    docker exec c0 bash run_cli.sh fetch run${node}_cli 
    cat scripts/deploy/run${node}_cli/remote/*/log/stderr | python3 scripts/thr_hist.py --plot >> ../logs/hotstuff/log_${node}.log
    # echo yunpeng | sudo -S rm -rf scripts/deploy/run1 scripts/deploy/run1_cli
    bash docker/kill_docker.sh 
    sleep 10
done