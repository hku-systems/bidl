#!/bin/bash -e

script_dir=$(cd "$(dirname "$0")";pwd)
source $script_dir/env.sh

bash $script_dir/start.sh 4 60 performance

total=$(tail -n 1 $log_dir/sequencer.log | awk -F ' ' '{print $2}')
recv=$(cat $log_dir/log_0.log | grep "TotalNum:" | tail -n 1 | awk -F ':' '{print $NF}')
echo "Total number of transactions: "$total
echo "Number of received transactions by nodes: "$recv
if [ $[$total-$recv] -gt 1000 ]
then 
	echo "Severe packet loss!"
else
	echo "Ok!"
fi