#!/bin/bash -e

echo  "Obtaining throughput and latency data..."

tput_file=$base_dir/logs/bidl_performance_tput.log
latency_file=$base_dir/logs/bidl_performance_latency.log

rm -f $tput_file
rm -f $latency_file

cat $log_dir/normal.log | grep "BIDL throughput" > $tput_file

cat $log_dir/log_0.log | grep "Total latency" > $latency_file