#!/bin/bash -e

echo  "Obtaining throughput and latency data..."

tput_file=$base_dir/logs/bidl_performance_tput.log
latency_file=$base_dir/logs/bidl_performance_latency.log

rm -f $tput_file
rm -f $latency_file

while true; do 
	wait=$( cat $log_dir/normal.log | grep "BIDL block commit throughput:" | wc -l)
	if [ $wait -gt 80 ]; then 
		break;
	fi 
	echo "wait 5s for results"
	sleep 5
done

cat $log_dir/normal.log | grep "BIDL transaction commit throughput: " > $tput_file

cat $log_dir/log_0.log | grep "Consensus latency" > $latency_file