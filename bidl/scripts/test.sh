#!/bin/bash -e

echo  "Obtaining throughput and latency data..."

base_dir="/home/jqi/github/BIDL-artifact/bidl"
log_dir="/home/jqi/github/BIDL-artifact/bidl/logs"

tput_file=$base_dir/logs/bidl_performance_tput.log
latency_file=$base_dir/logs/bidl_performance_latency.log

rm -f $tput_file
rm -f $latency_file

while true; do 
	wait=$( cat $log_dir/normal.log | grep "BIDL transaction commit throughput:" | wc -l)
	if [ $wait -gt 10 ]; then 
		break;
	fi 
	echo "wait 5s for results"
	sleep 5
done

# cat $log_dir/normal.log | grep "BIDL transaction commit throughput: " > $tput_file

# cat $log_dir/log_0.log | grep "Total latency" > $latency_file