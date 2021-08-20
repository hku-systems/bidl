#!/bin/bash
pkill -f bft
./runscripts/smartrun.sh bftsmart.demo.microbenchmarks.ThroughputLatencyServer 0 10 0 0 false nosig rwd >logs/logs_0.log 2>&1 &
./runscripts/smartrun.sh bftsmart.demo.microbenchmarks.ThroughputLatencyServer 1 100 0 0 false nosig rwd >logs/logs_1.log 2>&1 &
./runscripts/smartrun.sh bftsmart.demo.microbenchmarks.ThroughputLatencyServer 2 100 0 0 false nosig rwd >logs/logs_2.log 2>&1 &
./runscripts/smartrun.sh bftsmart.demo.microbenchmarks.ThroughputLatencyServer 3 100 0 0 false nosig rwd >logs/logs_3.log 2>&1 &