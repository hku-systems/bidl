#!/bin/bash

# Benchmark-related parameters
clients=10
#batches=(1 2 4 8 10 15 20)
batches=(10 20 50 100 200 500 1000)

. config.sh
./copy_files.sh

#. run_batchwrite/config_orderer.sh

./gen_scripts/gen_workload.sh "workloads/workload_load.template" "workload_load.in"
./gen_scripts/gen_workload.sh "workloads/read_insert/workload_10_10000.template" "workload.in"
size=$(grep -ch "^" workload_load.in)

for b in "${batches[@]}" ; do

	export STREAMCHAIN_WRITEBATCH=$b

    	flag="1"
	while [ $flag = "1" ] ; do

	./setup_network.sh
	./start_network.sh

	echo "Loading data.."
	$run_event "FABRIC_LOGGING_SPEC=CRITICAL ./peer chaincode invoke -c '{\"Args\":[\"invoke\"]}' -C mychannel -n YCSB --tls true --cafile=crypto-config/ordererOrganizations/example.com/orderers/orderer0.example.com/tls/ca.crt --workload=workload_load.in --parallelism=1 --repetitions=$size"
	sleep 40

	echo "Running benchmark: PEER_DATA=$peer_data STATEDB_DATA=$statedb_data ORDERER_DATA=$orderer_data PIPELINE=$STREAMCHAIN_PIPELINE WRITEBATCH=$STREAMCHAIN_WRITEBATCH"

	for e in $add_events ; do
		(ssh $user@$e "cd $bm_path ; FABRIC_LOGGING_SPEC=CRITICAL ./peer chaincode invoke -c '{\"Args\":[\"invoke\"]}' -C mychannel -n YCSB --tls true --cafile=crypto-config/ordererOrganizations/example.com/orderers/orderer0.example.com/tls/ca.crt --workload=workload.in --parallelism=$clients --repetitions=$(($size / $clients))") &
	done

	$run_event "FABRIC_LOGGING_SPEC=CRITICAL ./peer chaincode invoke -c '{\"Args\":[\"invoke\"]}' -C mychannel -n YCSB --tls true --cafile=crypto-config/ordererOrganizations/example.com/orderers/orderer0.example.com/tls/ca.crt --workload=workload.in --parallelism=$clients --repetitions=$(($size / $clients))"

	sleep 40
	flag=$?
	./teardown.sh
	sleep 5
	done
	./process_logs.sh $mode $clients $(($(wc -w <<< $add_events)+1)) $size $STREAMCHAIN_PIPELINE $b

done

echo 'Benchmark successfully completed'
