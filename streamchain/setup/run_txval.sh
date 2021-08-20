#!/bin/bash

. config.sh
./copy_files.sh

./gen_scripts/gen_workload.sh "workloads/workload_load.template" "workload_load.in"
size=$(grep -ch "^" workload_load.in)

# Benchmark-related parameters
clients=50

#for ratio in 0 10 50 90 100 ; do
for ratio in 50 ; do
	#for writeset in 10000 1000 100 ; do
	for writeset in 100 ; do

	    flag="1"
	    while [ $flag = "1" ] ; do
	    	./setup_network.sh
	    	./start_network.sh
	    	./gen_scripts/gen_workload.sh "workloads/read_modify_write/workload_${ratio}_${writeset}.template" "workload.in"

            	echo 'Loading data..'
       	    	$run_event "FABRIC_LOGGING_SPEC=CRITICAL ./peer chaincode invoke -c '{\"Args\":[\"invoke\"]}' -C mychannel -n YCSB --tls true --cafile=crypto-config/ordererOrganizations/example.com/orderers/orderer0.example.com/tls/ca.crt --workload=workload_load.in --parallelism=1 --repetitions=$size"
            	sleep 60

	    	echo "Running workload: workload_${ratio}_${writeset} with Mode=$mode #Tx/Block=$txs #Clients=$clients"

	    	$run_event "FABRIC_LOGGING_SPEC=CRITICAL ./peer chaincode invoke -c '{\"Args\":[\"invoke\"]}' -C mychannel -n YCSB --tls true --cafile=crypto-config/ordererOrganizations/example.com/orderers/orderer0.example.com/tls/ca.crt --workload=workload.in --parallelism=$clients --repetitions=$(($size / $clients))"

	    	sleep 60
		flag=$?
	    	./teardown.sh
		sleep 5
	    done
	    ./process_logs.sh $mode $clients $(($(wc -w <<< $add_events)+1)) $size $writeset $ratio
	done
done

echo 'Benchmark successfully completed'
