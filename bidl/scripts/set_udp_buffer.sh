#! /bin/bash
for i in `seq 20`; do
    echo "###  10.22.1.$i  ###"
	ssh -t sosp21ae@10.22.1.$i  "echo sosp21ae | sudo -S sysctl -w net.core.rmem_max=262144000"
done