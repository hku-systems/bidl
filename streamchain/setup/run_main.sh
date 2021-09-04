#!/bin/bash

. config.sh
./copy_files.sh

./gen_scripts/gen_workload.sh "workloads/workload_load.template" "workload_load.in"
./gen_scripts/gen_workload.sh "workloads/read_insert/workload_10_10000.template" "workload.in"
size=$(grep -ch "^" workload_load.in)

# Benchmark-related parameters
clients=(2 4 6 8 10 12)
txss=(1)

for tx in "${txss[@]}"; do

	export txs=$tx

	for j in "${clients[@]}"; do

	    flag="1"
	    while [ $flag = "1" ] ; do

		    ./setup_network.sh
		    ./start_network.sh

		    echo 'Loading data..'
	       	    $run_event "FABRIC_LOGGING_SPEC=CRITICAL ./peer chaincode invoke -c '{\"Args\":[\"invoke\"]}' -C mychannel -n YCSB --tls true --cafile=crypto-config/ordererOrganizations/example.com/orderers/orderer0.example.com/tls/ca.crt --workload=workload_load.in --parallelism=1 --repetitions=$size"
		    sleep 40

		    echo "Running benchmark: #Tx/Block=$tx #Clients=$j"

		    $run_event "FABRIC_LOGGING_SPEC=CRITICAL ./peer chaincode invoke -c '{\"Args\":[\"invoke\"]}' -C mychannel -n YCSB --tls true --cafile=crypto-config/ordererOrganizations/example.com/orderers/orderer0.example.com/tls/ca.crt --workload=workload.in --parallelism=$j --repetitions=$(($size / $j))"

		    sleep 40
		    flag=$?
		    ./teardown.sh
		    sleep 5
		done
		    ./process_logs.sh $mode $j $(($(wc -w <<< $add_events)+1)) $size $STREAMCHAIN_PIPELINE $tx
	done

done

echo 'Benchmark successfully completed'
