#!/bin/bash
set -e

echo  "Obtaining throughput and latency data..."

script_dir=$(cd "$(dirname "$0")";pwd)
source $script_dir/env.sh

tput_file=$base_dir/logs/bidl_performance_tput.log
latency_file=$base_dir/logs/bidl_performance_latency.log

rm -f $tput_file
rm -f $latency_file

while true; do 
	wait=$( cat $log_dir/normal_0.log | grep "BIDL block commit throughput:" | wc -l)
	if [ $wait -gt 50 ]; then 
		break;
	fi 
	echo "wait 5s for results"
	sleep 5
done

cat $log_dir/normal_0.log | grep "BIDL transaction commit throughput: " > $tput_file

cat $log_dir/consensus_0.log | grep "Consensus latency" > $latency_file