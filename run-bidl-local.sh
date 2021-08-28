#!/bin/bash -e

peers=4
default_tput=60

bash ./bidl/scripts/create_artifact.sh
if [ $1 == "performance" ]; then 
    rst_dir=./logs/bidl/performance
    rst_file=$rst_dir/performance.log
    rm -rf $rst_dir
    mkdir -p $rst_dir
    touch $rst_file
    for tput_cap in 60; do
    # for tput_cap in 50; do
        echo "Transaction submission rate: $tput_cap kTxns/s"
        # run benchmark
        bash ./bidl/scripts/start.sh $peers $tput_cap performance
        # obtain throughput data
        echo -n "rate $tput_cap throughput " >> $rst_file
        cat ./bidl/logs/normal_0.log | grep "BIDL transaction commit throughput:" | python3 ./bidl/scripts/bidl_tput.py $tput_cap >> $rst_file
        # obtain latency data
        # consensus latency
        echo -n "rate $tput_cap consensus latency " >> $rst_file
        cat ./bidl/logs/consensus_0.log | grep "Consensus latency" | python3 ./bidl/scripts/consensus_latency.py >> $rst_file
        # execution latency 
        echo -n "rate $tput_cap execution latency " >> $rst_file
        cat ./bidl/logs/normal_0.log | grep "Execution latency" | python3 ./bidl/scripts/bidl_latency.py >> $rst_file
        # commit latency 
        echo -n "rate $tput_cap commit latency " >> $rst_file
        cat ./bidl/logs/normal_0.log | grep "Commit latency" | python3 ./bidl/scripts/bidl_latency.py >> $rst_file
    done
    exit 0
elif [ $1 == "nd" ]; then 
    rst_dir=./logs/bidl/nondeterminism
    rst_file=$rst_dir/nondeterminism.log
    rm -rf $rst_dir
    mkdir -p $rst_dir
    touch $rst_file
    for nondeterminism_rate in 0 10 20 30 40 50; do 
        echo "Non-determinism rate: $nondeterminism_rate%"
        # run benchmark
        bash ./bidl/scripts/start.sh $peers $default_tput nd $nondeterminism_rate
        # obtain throughput data
        echo -n "rate $nondeterminism_rate throughput " >> $rst_file
        cat ./bidl/logs/normal_0.log | grep "BIDL transaction commit throughput" | python3 ./bidl/scripts/bidl_tput.py $default_tput >> $rst_file
    done
    exit 0
elif [ $1 == "contention" ]; then 
    rst_dir=./logs/bidl/contention
    rst_file=$rst_dir/contention.log
    rm -rf $rst_dir
    mkdir -p $rst_dir
    touch $rst_file
    for contention_rate in 10 20 30 40 50; do 
        echo "Contention rate = $contention_rate"
        # run benchmark
        bash ./bidl/scripts/start.sh $peers $default_tput contention $contention_rate 
        # obtain throughput data
        echo -n "rate $tput_cap throughput " >> $rst_file
        cat ./bidl/logs/normal_0.log | grep "BIDL throughput" | python3 ./bidl/scripts/bidl_tput.py $default_tput >> $rst_file
        # obtain latency data
        echo -n "rate $tput_cap latency " >> $rst_file
        cat ./bidl/logs/consensus_0.log | grep "Total latency" | python3 ./bidl/scripts/bidl_latency.py >> $rst_file
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
        echo -n "Orgs $org throughput " >> $rst_file
        cat ./bidl/logs/normal_0.log | grep "BIDL throughput" | python3 ./bidl/scripts/bidl_tput.py >> $rst_file
        # obtain latency data
        echo -n "Orgs $org latency " >> $rst_file
        cat ./bidl/logs/consensus_0.log | grep "Total latency" | python3 ./bidl/scripts/bidl_latency.py >> $rst_file
    done
    exit 0
else 
    echo "Invalid argument."
fi
