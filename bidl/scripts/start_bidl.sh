#!/bin/bash
set -u

if [ "$#" -lt 4 ]; then
    echo "Usage: bash ./bidl/scripts/start_bidl.sh <1. num of consensus nodes> <2. num of normal nodes> <3. peak throughput> <4. benchmark> <5. benchmark parameters*>"
    exit 1
fi

script_dir=$(cd "$(dirname "$0")";pwd)
source $script_dir/env.sh

echo "Stoping all BIDL nodes."
bash $base_dir/scripts/kill_all.sh

host_num=`cat $base_dir/scripts/servers|wc -l`
if [ $host_num -gt $1 ]; then
    host_num=$1
fi
consensus_nodes_per_host=$[$1/$host_num]
extra_nodes=$[$1 - $host_num*$consensus_nodes_per_host]
echo "Consensus nodes per host is $consensus_nodes_per_host, and $extra_nodes extra consensus nodes will be deployed on the last host."

echo "######################################################"
echo "Start $1 consensus nodes on $host_num servers"
echo "######################################################"

i=0
for host in `cat $base_dir/scripts/servers`; do
    ssh -n ${USER}@${host} "cd ~; rm -rf ~/logs; mkdir ~/logs;"
    for node in `seq 0 $[${consensus_nodes_per_host}-1]`; do
        echo ${host} ${i}
        ssh -n ${USER}@${host} "docker run --name smart$i --net=host --cap-add NET_ADMIN smart bash /home/runscripts/smartrun.sh bftsmart.demo.microbenchmarks.ThroughputLatencyServer $i 10 0 0 false nosig rwd > logs/consensus_$i.log 2>&1 &"
        let i=$i+1
    done
    if [ $i -eq $1 ]; then
        break
    fi
done 

for node in `seq 0 $[${extra_nodes}-1]`; do
    echo "Extra consensus nodes on localhost:"${i}
    ssh -n ${USER}@${host} "docker run --name smart$i --net=host --cap-add NET_ADMIN smart bash /home/runscripts/smartrun.sh bftsmart.demo.microbenchmarks.ThroughputLatencyServer $i 10 0 0 false nosig rwd > logs/consensus_$i.log 2>&1 &"
    let i=$i+1
done

i=0
while true; do 
    let i=$i+1
	wait=$( cat /home/$USER/logs/consensus_0.log | grep "Ready to process operations" | wc -l)
	if [ $wait -eq 1 ]; then 
		break;
	fi 
	if [ $i -gt 10 ]; then 
        echo "reaching maximum wait time, continue."
		break;
	fi 
	echo "wait 5s for consensus nodes to setup"
	sleep 5
done

################################################################################

host_num=`cat $base_dir/scripts/servers|wc -l`
if [ $host_num -gt $2 ]; then
    host_num=$2
fi
normal_nodes_per_host=$[$2/$host_num]

echo "Normal nodes per host is $normal_nodes_per_host"
echo "######################################################"
echo "Start $2 normal nodes on $host_num servers"
echo "######################################################"

i=0
for host in `cat $base_dir/scripts/servers`; do
    for node in `seq 0 $[${normal_nodes_per_host}-1]`; do
        echo ${host} ${i}
        ssh -n ${USER}@${host} "docker run --name normal_node$i --net=host --cap-add NET_ADMIN normal_node /normal_node/server --quiet --tps=$3 --id=$i > logs/normal_${i}.log 2>&1 &"
        let i=$i+1
    done
done 

docker run --name sequencer --net=host sequencer:latest /sequencer/sequencer $3 > /home/${USER}/logs/sequencer.log 2>&1 &

echo "benchmarking..."
cd $normal_node_dir
if [ $4 == "performance" ]; then
    docker run --name bidl_client --net=host --cap-add NET_ADMIN normal_node /normal_node/client --num=100000 --org=$2 --quiet
elif [ $4 == "nd" ]; then 
    docker run --name bidl_client --net=host --cap-add NET_ADMIN normal_node /normal_node/client --num=100000 --org=$2 --nd=$5 --quiet
elif [ $4 == "contention" ]; then 
    docker run --name bidl_client --net=host --cap-add NET_ADMIN normal_node /normal_node/client --num=100000 --org=$2 --conflict=$5 --quiet
elif [ $4 == "scalability" ]; then 
    docker run --name bidl_client --net=host --cap-add NET_ADMIN normal_node /normal_node/client --num=100000 --org=$2 --quiet
else 
    echo "Invalid argument."
    exit 1
fi

cd $base_dir

while true; do 
	wait=$( cat /home/$USER/logs/normal_0.log | grep "BIDL block commit throughput:" | wc -l)
	if [ $wait -gt 50 ]; then 
		break;
	fi 
	echo "wait 5s for results"
	sleep 5
done

