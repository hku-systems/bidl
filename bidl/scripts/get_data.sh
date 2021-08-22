#!/bin/bash -e

echo  "Obtaining throughput and latency data..."

file=$base_dir/logs/bidl_performance_$1.log

rm -f $file

cat $log_dir/normal.log | grep "BIDL throughput" > $file

cat $log_dir/log_0.log | grep "Total latency" >> $file