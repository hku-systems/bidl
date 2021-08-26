#!/bin/bash -e

peers=4

bash ./bidl/scripts/create_artifact.sh
if [ $1 == "performance" ]; then 
    rst_dir=./logs/bidl/performance
    rst_file=$rst_dir/performance.log
    rm -rf $rst_dir
    mkdir -p $rst_dir
    touch $rst_file
    for tput_cap in 15 30 60; do
        echo "Transaction submission rate: $nondeterminism_rate kTxns/s"
        # run benchmark
        bash ./bidl/scripts/start.sh $peers $tput_cap performance
        # obtain throughput data
        echo -n "rate $tput_cap throughput " >> $rst_file
        cat ./bidl/logs/normal.log | grep "BIDL block commit throughput:" | python3 ./bidl/scripts/bidl_tput.py >> $rst_file
        # obtain latency data
        echo -n "rate $tput_cap latency " >> $rst_file
        # cat ./bidl/logs/log_0.log | grep "Total latency" | python3 ./bidl/scripts/bidl_latency.py >> $rst_file
        cat ./bidl/logs/log_0.log | grep "Consensus latency" | python3 ./bidl/scripts/bidl_latency.py >> $rst_file
    done
    exit 0
elif [ $1 == "nd" ]; then 
    rst_dir=./logs/bidl/nondeterminism
    rst_file=$rst_dir/nondeterminism.log
    rm -rf $rst_dir
    mkdir -p $rst_dir
    touch $rst_file
    for nondeterminism_rate in 0; do 
        echo "Non-determinism rate: $nondeterminism_rate%"
        # run benchmark
        bash ./bidl/scripts/start.sh $peers 60 nd $nondeterminism_rate
        # obtain throughput data
        echo -n "rate $tput_cap throughput " >> $rst_file
        cat ./bidl/logs/normal.log | grep "BIDL transaction commit throughput" | python3 ./bidl/scripts/bidl_tput.py >> $rst_file
        # obtain latency data
        echo -n "rate $tput_cap latency " >> $rst_file
        cat ./bidl/logs/log_0.log | grep "Total latency" | python3 ./bidl/scripts/bidl_latency.py >> $rst_file
    done
    exit 0
elif [ $1 == "contention" ]; then 
    rst_dir=./logs/bidl/contention
    rst_file=$rst_dir/contention.log
    rm -rf $rst_dir
    mkdir -p $rst_dir
    touch $rst_file
    for contention_rate in 0.1 0.2 0.3 0.4 0.5; do 
        echo "Contention rate = $contention_rate"
        # run benchmark
        bash ./bidl/scripts/start.sh $peers 60 contention $contention_rate 
        # obtain throughput data
        echo -n "rate $tput_cap throughput " >> $rst_file
        cat ./bidl/logs/normal.log | grep "BIDL throughput" | python3 ./bidl/scripts/bidl_tput.py >> $rst_file
        # obtain latency data
        echo -n "rate $tput_cap latency " >> $rst_file
        cat ./bidl/logs/log_0.log | grep "Total latency" | python3 ./bidl/scripts/bidl_latency.py >> $rst_file
    done
    exit 0
elif [ $1 == "scalability" ]; then 
    rst_dir=./logs/bidl/scalability
    rst_file=$rst_dir/scalability.log
    rm -rf $rst_dir
    mkdir -p $rst_dir
    touch $rst_file
    for org in 4 7 13; do 
        echo "Number of organizations = $org"
        # run benchmark
        bash ./bidl/scripts/start.sh $org 60 scalability $org 
        # obtain throughput data
        echo -n "rate $tput_cap throughput " >> $rst_file
        cat ./bidl/logs/normal.log | grep "BIDL throughput" | python3 ./bidl/scripts/bidl_tput.py >> $rst_file
        # obtain latency data
        echo -n "rate $tput_cap latency " >> $rst_file
        cat ./bidl/logs/log_0.log | grep "Total latency" | python3 ./bidl/scripts/bidl_latency.py >> $rst_file
    done
    exit 0
else 
    echo "Invalid argument."
fi
