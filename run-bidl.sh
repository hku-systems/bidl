#!/bin/bash 
set -u
default_peers=4
default_tput=50
bash ./bidl/scripts/kill_all.sh
# bash ./bidl/scripts/deploy_bidl.sh $default_peers

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
    bash ./bidl/scripts/kill_all.sh
    cat $rst_file
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
    bash ./bidl/scripts/kill_all.sh
    cat $rst_file
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
        echo -n "rate $contention_rate throughput " >> $rst_file
        cat /home/$USER/logs/normal_0.log | grep "BIDL transaction commit throughput" | python3 ./bidl/scripts/bidl_tput.py $default_tput >> $rst_file
    done
    bash ./bidl/scripts/kill_all.sh
    cat $rst_file
    exit 0
elif [ $1 == "scalability" ]; then 
    rst_dir=./logs/bidl/scalability
    rst_file=$rst_dir/scalability.log
    rm -rf $rst_dir
    mkdir -p $rst_dir
    touch $rst_file
    # generate config file for all settings
    for org in 4 7 13 25 49; do 
        bash ./bidl/scripts/gen_host_conf.sh $org
        cp ./bidl/consensus_node/bftsmart/config/hosts.config ./bidl/scripts/configs/hosts_$org.config
    done
    bash ./bidl/scripts/copy_smart_config.sh

    for org in 4 7 13 25 49; do 
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
    cat $rst_file
    exit 0
elif [ $1 == "malicious" ]; then 
    rst_dir=./logs/bidl/malicious
    rst_file=$rst_dir/malicious.log
    rm -rf $rst_dir
    mkdir -p $rst_dir
    touch $rst_file
    bash ./bidl/scripts/start_local_only_consensus.sh $default_peers $default_tput malicious
    send_num=260000
    for view in 0; do # misbehave
        # run benchmark
        bash ./bidl/scripts/benchmark.sh 50 malicious $(( $send_num * $view )) 

        # view change
        new_view=$(( $view + 1 ))
        nodeID=$(( $new_view % $default_peers ))
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
        # run benchmark
        bash ./bidl/scripts/benchmark.sh 50 performance $(( $send_num * $view )) 

        new_view=$(( $view + 1 ))
        nodeID=$(( $new_view % $default_peers ))
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
    for view in 2 3; do # misbehave
        # run benchmark
        bash ./bidl/scripts/benchmark.sh 50 malicious $(( $send_num * $view )) 

        new_view=$(( $view + 1 ))
        nodeID=$(( $new_view % $default_peers ))
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
    for view in 4; do # misbehave
        # run benchmark
        bash ./bidl/scripts/benchmark.sh 50 malicious $(( $send_num * $view )) 
        sleep 10
    done
    bash ./bidl/scripts/kill_all_local.sh
    exit 0
else 
    echo "Invalid argument."
fi
