#!/bin/bash -e

default_peers=4
default_tput=60
bash ./bidl/scripts/kill_all.sh
bash ./bidl/scripts/deploy_bidl.sh $default_peers

if [ $1 == "performance" ]; then 
    rst_dir=./logs/bidl/performance
    rst_file=$rst_dir/performance.log
    rm -rf $rst_dir
    mkdir -p $rst_dir
    touch $rst_file
    for tput_cap in 20 40 60; do
        echo "Transaction submission rate: $tput_cap kTxns/s"
        # run benchmark
        bash ./bidl/scripts/start_bidl.sh 4 50 $tput_cap performance
        # obtain throughput data
        echo -n "rate $tput_cap throughput " >> $rst_file
        cat /home/$USER/logs/normal_0.log | grep "BIDL transaction commit throughput:" | python3 ./bidl/scripts/bidl_tput.py $tput_cap >> $rst_file
        # obtain latency data
        # consensus latency
        echo -n "rate $tput_cap consensus latency " >> $rst_file
        cat /home/$USER/logs/consensus_0.log | grep "Consensus latency" | python3 ./bidl/scripts/consensus_latency.py >> $rst_file
        # execution latency 
        echo -n "rate $tput_cap execution latency " >> $rst_file
        cat /home/$USER/logs/normal_0.log | grep "Execution latency" | python3 ./bidl/scripts/bidl_latency.py >> $rst_file
        # commit latency 
        echo -n "rate $tput_cap commit latency " >> $rst_file
        cat /home/$USER/logs/normal_0.log | grep "Commit latency" | python3 ./bidl/scripts/bidl_latency.py >> $rst_file
    done
    source $base_dir/scripts/kill_all.sh
    exit 0
elif [ $1 == "nd" ]; then 
    rst_dir=./logs/bidl/nondeterminism
    rst_file=$rst_dir/nondeterminism.log
    rm -rf $rst_dir
    mkdir -p $rst_dir
    touch $rst_file
    for nondeterminism_rate in 0 25 50; do 
        echo "Non-determinism rate: $nondeterminism_rate%"
        # run benchmark
        bash ./bidl/scripts/start_bidl.sh 4 50 $default_tput nd $nondeterminism_rate
        # obtain throughput data
        echo -n "rate $nondeterminism_rate throughput " >> $rst_file
        cat /home/$USER/logs/normal_0.log | grep "BIDL transaction commit throughput" | python3 ./bidl/scripts/bidl_tput.py $default_tput >> $rst_file
    done
    source $base_dir/scripts/kill_all.sh
    exit 0
elif [ $1 == "contention" ]; then 
    rst_dir=./logs/bidl/contention
    rst_file=$rst_dir/contention.log
    rm -rf $rst_dir
    mkdir -p $rst_dir
    touch $rst_file
    for contention_rate in 0 25 50; do 
        echo "Contention rate: $contention_rate"
        # run benchmark
        bash ./bidl/scripts/start_bidl.sh 4 50 $default_tput contention $contention_rate 
        # obtain throughput data
        echo -n "rate $tput_cap throughput " >> $rst_file
        cat /home/$USER/logs/normal_0.log | grep "BIDL transaction commit throughput" | python3 ./bidl/scripts/bidl_tput.py $default_tput >> $rst_file
    done
    source $base_dir/scripts/kill_all.sh
    exit 0
elif [ $1 == "scalability" ]; then 
    rst_dir=./logs/bidl/scalability
    rst_file=$rst_dir/scalability.log
    rm -rf $rst_dir
    mkdir -p $rst_dir
    touch $rst_file
    # generate config file for all settings
    for org in 4 25 49; do 
        bash ./bidl/scripts/gen_host_conf.sh $org
        cp ./bidl/consensus_node/bftsmart/config/hosts.config ./bidl/scripts/configs/hosts_$org.config
    done
    bash ./bidl/scripts/copy_smart_config.sh

    for org in 4 25 49; do 
        echo "Number of organizations = $org"
        # run benchmark
        bash ./bidl/scripts/start_bidl_scalability.sh $org $org $default_tput scalability
        # obtain latency data
        # consensus latency
        echo -n "rate $org consensus latency " >> $rst_file
        cat /home/$USER/logs/consensus_0.log | grep "Consensus latency" | python3 ./bidl/scripts/consensus_latency.py >> $rst_file
        # execution latency 
        echo -n "rate $org execution latency " >> $rst_file
        cat /home/$USER/logs/normal_0.log | grep "Execution latency" | python3 ./bidl/scripts/bidl_latency.py >> $rst_file
        # commit latency 
        echo -n "rate $org commit latency " >> $rst_file
        cat /home/$USER/logs/normal_0.log | grep "Commit latency" | python3 ./bidl/scripts/bidl_latency.py >> $rst_file
    done
    bash ./bidl/scripts/kill_all.sh
    exit 0
else 
    echo "Invalid argument."
fi
