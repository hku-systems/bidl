#!/bin/bash
set -u
peers=4 # number of consensus nodes
default_tput=40 # trnasaction submission tput
bash ./bidl/scripts/create_artifact.sh # build BIDL images

if [ $1 == "test" ]; then
    rst_dir=./logs/bidl/test
    rst_file=$rst_dir/test.log
    rm -rf $rst_dir
    mkdir -p $rst_dir
    touch $rst_file
    # start four consensus node and one normal node locally
    bash ./bidl/scripts/start_local.sh $peers $default_tput test
    # throughput 
    echo -n "Submission rate $default_tput throughput " >> $rst_file
    cat ./bidl/logs/normal_0.log | grep "BIDL transaction commit throughput:" | python3 ./bidl/scripts/bidl_tput.py $default_tput >> $rst_file
    # consensus latency
    echo -n "Submission rate $default_tput consensus latency " >> $rst_file
    cat ./bidl/logs/consensus_0.log | grep "Consensus latency" | python3 ./bidl/scripts/consensus_latency.py >> $rst_file
    # execution latency 
    echo -n "Submission rate $default_tput execution latency " >> $rst_file
    cat ./bidl/logs/normal_0.log | grep "Execution latency" | python3 ./bidl/scripts/bidl_latency.py >> $rst_file
    # commit latency 
    echo -n "Submission rate $default_tput commit latency " >> $rst_file
    cat ./bidl/logs/normal_0.log | grep "Commit latency" | python3 ./bidl/scripts/bidl_latency.py >> $rst_file
    bash ./bidl/scripts/kill_all_local.sh
    echo "#################################################"
    cat $rst_file
    echo "#################################################"
elif [ $1 == "performance" ]; then 
    rst_dir=./logs/bidl/performance
    rst_file=$rst_dir/performance.log
    rm -rf $rst_dir
    mkdir -p $rst_dir
    touch $rst_file
    for tput_cap in 20 40 60; do
        echo "Transaction submission rate: $tput_cap kTxns/s"
        # run benchmark
        bash ./bidl/scripts/start_local.sh $peers $tput_cap performance
        # throughput 
        echo -n "rate $tput_cap throughput " >> $rst_file
        cat ./bidl/logs/normal_0.log | grep "BIDL transaction commit throughput:" | python3 ./bidl/scripts/bidl_tput.py $tput_cap >> $rst_file
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
    bash ./bidl/scripts/kill_all_local.sh
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
        bash ./bidl/scripts/start_local.sh $peers $default_tput nd $nondeterminism_rate
        # obtain throughput data
        echo -n "rate $nondeterminism_rate throughput " >> $rst_file
        cat ./bidl/logs/normal_0.log | grep "BIDL transaction commit throughput" | python3 ./bidl/scripts/bidl_tput.py $default_tput >> $rst_file
    done
    bash ./bidl/scripts/kill_all_local.sh
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
        bash ./bidl/scripts/start_local.sh $peers $default_tput contention $contention_rate 
        # obtain throughput data
        echo -n "rate $tput_cap throughput " >> $rst_file
        cat ./bidl/logs/normal_0.log | grep "BIDL throughput" | python3 ./bidl/scripts/bidl_tput.py $default_tput >> $rst_file
        # obtain latency data
        echo -n "rate $tput_cap latency " >> $rst_file
        cat ./bidl/logs/consensus_0.log | grep "Total latency" | python3 ./bidl/scripts/bidl_latency.py >> $rst_file
    done
    bash ./bidl/scripts/kill_all_local.sh
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
        bash ./bidl/scripts/start_local.sh $org 60 scalability $org 
        # obtain throughput data
        echo -n "Orgs $org throughput " >> $rst_file
        cat ./bidl/logs/normal_0.log | grep "BIDL throughput" | python3 ./bidl/scripts/bidl_tput.py >> $rst_file
        # obtain latency data
        echo -n "Orgs $org latency " >> $rst_file
        cat ./bidl/logs/consensus_0.log | grep "Total latency" | python3 ./bidl/scripts/bidl_latency.py >> $rst_file
    done
    bash ./bidl/scripts/kill_all_local.sh
    exit 0
elif [ $1 == "malicious" ]; then 
    rst_dir=./logs/bidl/malicious
    rst_file=$rst_dir/malicious.log
    rm -rf $rst_dir
    mkdir -p $rst_dir
    touch $rst_file
    bash ./bidl/scripts/start_local_test.sh $peers $default_tput malicious
    send_num=260000
    # send_num=510000
    for view in 0; do
        # kill client
        docker stop $(docker ps -aq --filter name="bidl_client"); docker rm $(docker ps -aq --filter name="bidl_client")

        # run benchmarking
        bash ./bidl/scripts/benchmark.sh 50 malicious $(( $send_num * $view )) 

        new_view=$(( $view + 1 ))
        nodeID=$(( $new_view % $peers ))
        echo "The new leader for the next view $new_view is node $nodeID"
        while true; do
            wait=$( cat ./bidl/logs/consensus_${nodeID}.log | grep "I'm the new leader for regency ${new_view}" | wc -l)
            if [ $wait -eq 1 ]; then
                break;
            fi
            echo "Wait 5s for consensus nodes to view change"
            sleep 5
        done
        sleep 10
    done
    for view in 1; do
        # kill client
        docker stop $(docker ps -aq --filter name="bidl_client"); docker rm $(docker ps -aq --filter name="bidl_client")

        # run benchmarking
        bash ./bidl/scripts/benchmark.sh 50 performance $(( $send_num * $view )) 

        new_view=$(( $view + 1 ))
        nodeID=$(( $new_view % $peers ))
        echo "The new leader for the next view $new_view is node $nodeID"
        while true; do
            wait=$( cat ./bidl/logs/consensus_${nodeID}.log | grep "I'm the new leader for regency ${new_view}" | wc -l)
            if [ $wait -eq 1 ]; then
                break;
            fi
            echo "Wait 5s for consensus nodes to view change"
            sleep 5
        done
        sleep 10
    done
    for view in 2 3 4; do
        # kill client
        docker stop $(docker ps -aq --filter name="bidl_client"); docker rm $(docker ps -aq --filter name="bidl_client")

        # run benchmarking
        bash ./bidl/scripts/benchmark.sh 50 malicious $(( $send_num * $view )) 

        new_view=$(( $view + 1 ))
        nodeID=$(( $new_view % $peers ))
        echo "The new leader for the next view $new_view is node $nodeID"
        while true; do
            wait=$( cat ./bidl/logs/consensus_${nodeID}.log | grep "I'm the new leader for regency ${new_view}" | wc -l)
            if [ $wait -eq 1 ]; then
                break;
            fi
            echo "Wait 5s for consensus nodes to view change"
            sleep 5
        done
        sleep 10
    done
    bash ./bidl/scripts/kill_all_local.sh
    exit 0
else 
    echo "Invalid argument."
fi
