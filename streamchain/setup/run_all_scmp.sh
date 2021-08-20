#!/bin/bash

. config.sh

clients=(1 2 5 10 20)

./copy_files.sh 'chaincode_mgr.go'

for c in ${clients[@]} ; do
for workload in "medium" ; do

    cp -rf workloads/supplychain/$workload/* workloads/

    for type in $(find workloads/supplychain/$workload -mindepth 1 -maxdepth 1 -type d -printf '%f\n') ; do

        ./run_main_scmp.sh $type $c

    done
done
done
