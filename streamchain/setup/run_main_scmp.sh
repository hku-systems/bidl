#!/bin/bash

type=$1
clients=$2

./gen_scripts/gen_workload.sh "workloads/$type/workload-load-1.template" "workloads/$type/workload-load-1.in"
./gen_scripts/gen_workload.sh "workloads/$type/workload-load-2.template" "workloads/$type/workload-load-2.in"
./gen_scripts/gen_workload.sh "workloads/$type/workload-load-3.template" "workloads/$type/workload-load-3.in"
./gen_scripts/gen_workload.sh "workloads/$type/workload-load-4.template" "workloads/$type/workload-load-4.in"
./gen_scripts/gen_workload.sh "workloads/$type/workload-load-5.template" "workloads/$type/workload-load-5.in"
./gen_scripts/gen_workload.sh "workloads/$type/workload-benchmark.template" "workloads/$type/workload-benchmark.in"

flag=1
while [ $flag != 0 ]; do

	timestamp=$(date +"%m-%d-%y-%H-%M")

	./setup_network.sh
	./start_network.sh

	echo "Running benchmark: #Tx/Block=$txs #Clients=$clients"
	echo "Running LOAD 1"
	size=$(grep -ch "^" workloads/$type/workload-load-1.in)
	$run_event "FABRIC_LOGGING_SPEC=CRITICAL ./peer chaincode invoke -c '{\"Args\":[\"invoke\"]}' -C mychannel -n YCSB --tls true --cafile=crypto-config/ordererOrganizations/example.com/orderers/orderer0.example.com/tls/ca.crt --workload=workload-load-1.in --parallelism=1 --repetitions=$size"

	echo "Running LOAD 2"
	size=$(grep -ch "^" workloads/$type/workload-load-2.in)
	$run_event "FABRIC_LOGGING_SPEC=CRITICAL ./peer chaincode invoke -c '{\"Args\":[\"invoke\"]}' -C mychannel -n YCSB --tls true --cafile=crypto-config/ordererOrganizations/example.com/orderers/orderer0.example.com/tls/ca.crt --workload=workload-load-2.in --parallelism=1 --repetitions=$(($size))"

	echo "Running LOAD 3"
	size=$(grep -ch "^" workloads/$type/workload-load-3.in)
	$run_event "FABRIC_LOGGING_SPEC=CRITICAL ./peer chaincode invoke -c '{\"Args\":[\"invoke\"]}' -C mychannel -n YCSB --tls true --cafile=crypto-config/ordererOrganizations/example.com/orderers/orderer0.example.com/tls/ca.crt --workload=workload-load-3.in --parallelism=1 --repetitions=$(($size))"

	echo "Running LOAD 4"
	size=$(grep -ch "^" workloads/$type/workload-load-4.in)
	$run_event "FABRIC_LOGGING_SPEC=CRITICAL ./peer chaincode invoke -c '{\"Args\":[\"invoke\"]}' -C mychannel -n YCSB --tls true --cafile=crypto-config/ordererOrganizations/example.com/orderers/orderer0.example.com/tls/ca.crt --workload=workload-load-4.in --parallelism=1 --repetitions=$(($size))"

	echo "Running LOAD 5"
	size=$(grep -ch "^" workloads/$type/workload-load-5.in)
	$run_event "FABRIC_LOGGING_SPEC=CRITICAL ./peer chaincode invoke -c '{\"Args\":[\"invoke\"]}' -C mychannel -n YCSB --tls true --cafile=crypto-config/ordererOrganizations/example.com/orderers/orderer0.example.com/tls/ca.crt --workload=workload-load-5.in --parallelism=1 --repetitions=$(($size))"

	# ADJUST THIS WAITING TIME FOR LOADING #

	sleep 200

	echo "Running BENCHMARK"
	size=$(grep -ch "^" workloads/$type/workload-benchmark.in)
	$run_event "FABRIC_LOGGING_SPEC=CRITICAL ./peer chaincode invoke -c '{\"Args\":[\"invoke\"]}' -C mychannel -n YCSB --tls true --cafile=crypto-config/ordererOrganizations/example.com/orderers/orderer0.example.com/tls/ca.crt --workload=workload-benchmark.in --parallelism=$clients --repetitions=$(($size / $clients))"
	flag=$?

	# ADJUST THIS WAITING TIME FOR BENCHMARK #

	sleep 200

	if [ $flag -eq 0 ] ; then
		./process_logs.sh "$mode-$type" $clients $(($(wc -w <<< $add_events)+1)) $size $txs 'Benchmark' $timestamp
	fi
	./teardown.sh
done
sleep 5

echo 'Benchmark successfully completed'
