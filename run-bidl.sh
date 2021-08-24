#!/bin/bash -e

peers=4

if [ $1 == "performance" ]; then 
    bash ./bidl/scripts/create_artifact.sh
    log_dir=./logs/bidl/performance
    log_file=$log_dir/performance.log
    rm -rf $log_dir
    mkdir -p $log_dir
    touch $log_file
    for tput_cap in 10 20 40 60; do
        echo "Transaction submission rate: $nondeterminism_rate kTxns/s"
        # run benchmark
        bash ./bidl/scripts/start.sh $peers $tput_cap
        # obtain throughput data
        echo -n "rate $tput_cap throughput " >> $log_file
        cat ./bidl/logs/normal.log | grep "BIDL throughput" | python3 ./bidl/scripts/bidl_tput.py >> $log_file
        # obtain latency data
        echo -n "rate $tput_cap latency " >> $log_file
        cat ./bidl/logs/log_0.log | grep "Total latency" | python3 ./bidl/scripts/bidl_latency.py >> $log_file
    done
    exit 0
elif [ $1 == "nd" ]; then 
    log_dir=./logs/bidl/nondeterminism
    log_file=$log_dir/nondeterminism.log
    rm -rf $log_dir
    mkdir -p $log_dir
    touch $log_file
    for nondeterminism_rate in 10 20 30 40 50; do 
        echo "Non-determinism rate: $nondeterminism_rate%"
        # run benchmark
        bash ./bidl/scripts/start.sh $peers 60 nd $nondeterminism_rate
        # obtain throughput data
        echo -n "rate $tput_cap throughput " >> $log_file
        cat ./bidl/logs/normal.log | grep "BIDL throughput" | python3 ./bidl/scripts/bidl_tput.py >> $log_file
        # obtain latency data
        echo -n "rate $tput_cap latency " >> $log_file
        cat ./bidl/logs/log_0.log | grep "Total latency" | python3 ./bidl/scripts/bidl_latency.py >> $log_file
    done
    exit 0
elif [ $1 == "contention" ]; then 
    log_dir=./logs/bidl/contention
    log_file=$log_dir/contention.log
    rm -rf $log_dir
    mkdir -p $log_dir
    touch $log_file
    for contention_rate in 0.1 0.2 0.3 0.4 0.5; do 
        echo "Contention rate = $contention_rate"
        # run benchmark
        bash ./bidl/scripts/start.sh $peers 60 contention $contention_rate 
        # obtain throughput data
        echo -n "rate $tput_cap throughput " >> $log_file
        cat ./bidl/logs/normal.log | grep "BIDL throughput" | python3 ./bidl/scripts/bidl_tput.py >> $log_file
        # obtain latency data
        echo -n "rate $tput_cap latency " >> $log_file
        cat ./bidl/logs/log_0.log | grep "Total latency" | python3 ./bidl/scripts/bidl_latency.py >> $log_file
    done
    exit 0
elif [ $1 == "scalability" ]; then 
    log_dir=./logs/bidl/scalability
    log_file=$log_dir/scalability.log
    rm -rf $log_dir
    mkdir -p $log_dir
    touch $log_file
    for org in 4 7 13; do 
        echo "Number of organizations = $org"
        # run benchmark
        bash ./bidl/scripts/start.sh $org 60 scalability $org 
        # obtain throughput data
        echo -n "rate $tput_cap throughput " >> $log_file
        cat ./bidl/logs/normal.log | grep "BIDL throughput" | python3 ./bidl/scripts/bidl_tput.py >> $log_file
        # obtain latency data
        echo -n "rate $tput_cap latency " >> $log_file
        cat ./bidl/logs/log_0.log | grep "Total latency" | python3 ./bidl/scripts/bidl_latency.py >> $log_file
    done
    exit 0
else 
    echo "Invalid argument."
fi
