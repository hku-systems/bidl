#!/bin/bash

# Benchmark-related parameters
clients=10
#pipeline=(0 1 2 4 6 8 16)
pipeline=(16)

. config.sh
./copy_files.sh

./gen_scripts/gen_workload.sh "workloads/workload_load.template" "workload_load.in"
./gen_scripts/gen_workload.sh "workloads/workload_10.template" "workload.in"
size=$(grep -ch "^" workload.in)

run() {
	for p in "${pipeline[@]}" ; do

        	export STREAMCHAIN_PIPELINE=$p
			    
		flag="1"
	    	while [ $flag = "1" ] ; do

			./setup_network.sh
			./start_network.sh

			echo "Loading data.."
			$run_event "FABRIC_LOGGING_SPEC=CRITICAL ./peer chaincode invoke -c '{\"Args\":[\"invoke\"]}' -C mychannel -n YCSB --tls true --cafile=crypto-config/ordererOrganizations/example.com/orderers/orderer0.example.com/tls/ca.crt --workload=workload_load.in --parallelism=1 --repetitions=$size"
			
			sleep 60

			echo "Running benchmark: PEER_DATA=$peer_data ORDERER_DATA=$orderer_data PIPELINE=$STREAMCHAIN_PIPELINE"

			for e in $add_events ; do
				(ssh $user@$e "FABRIC_LOGGING_SPEC=CRITICAL ./peer chaincode invoke -c '{\"Args\":[\"invoke\"]}' -C mychannel -n YCSB --tls true --cafile=crypto-config/ordererOrganizations/example.com/orderers/orderer0.example.com/tls/ca.crt --workload=workload.in --parallelism=$clients --repetitions=$(($size / $clients))") &
			done

			$run_event "FABRIC_LOGGING_SPEC=CRITICAL ./peer chaincode invoke -c '{\"Args\":[\"invoke\"]}' -C mychannel -n YCSB --tls true --cafile=crypto-config/ordererOrganizations/example.com/orderers/orderer0.example.com/tls/ca.crt --workload=workload.in --parallelism=$clients --repetitions=$(($size / $clients))"

			sleep 60
			flag=$?
			./teardown.sh
			sleep 5
		done
			./process_logs.sh $mode $clients $size $(($(wc -w <<< $add_events)+1)) $p $1
	done
}

#1 Peer commits to disk
. run_pipeline/config_peer.sh && run 'a'

#2 Orderer commits to disk
. run_pipeline/config_orderer.sh && run 'b'

#3 Streamchain
. run_pipeline/config_str.sh && run 'c'

echo 'Benchmark successfully completed'
