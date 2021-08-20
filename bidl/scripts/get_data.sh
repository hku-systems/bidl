#!/bin/bash -e

echo  "Obtaining throughput and latency data..."

cat $log_dir/normal.log | grep "BIDL throughput" 

cat $log_dir/log_0.log | grep "Total latency" 