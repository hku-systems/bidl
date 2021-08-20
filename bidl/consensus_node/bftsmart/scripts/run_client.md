
```
ThroughputLatencyClient 
<initial client id> 
<number of clients> <number of operations> 
<request size> <interval (ms)> <read only?> <verbose?> 
<nosig | default | ecdsa>
```

In our evaluation, the front end collects transactions from sequencer, and
forawrds the packaged block to the leader for consensus. Thus, there is only one
client.

```
./runscripts/smartrun.sh bftsmart.demo.microbenchmarks.ThroughputLatencyClient 1001 10 500 20000 0 false false nosig

./runscripts/smartrun.sh bftsmart.demo.microbenchmarks.ThroughputLatencyClient 1001 1 500 512000 0 false false nosig

./runscripts/smartrun.sh bftsmart.demo.microbenchmarks.ThroughputLatencyClient 1001 1 500 2048000 0 false false nosig
```
