# Get Started Tutorial

## Run a FastFabric Network  

You can bring the FastFabric network up with the following command:

```bash
bash example.sh 
```

The above command will:

1. start a FastFabric network with one orderer and five peers
2. create a channel
3. deploy the smallbank smart contract: `chaincode/smallbank/smallbank.go`
4. create 5000 accounts
5. send 5000 payment transactions

Upon successful completion, the logs should report the following in your terminal window:

```bash
#################################################
create 5000 acocunts
tx: 50000, duration: 2.544193932s, tps: 19652.589911
start 5000 payment transactions
tx: 50000, duration: 3.229338253s, tps: 15483.048254
valid: 40763/50000 81.53% invalid: 9237/50000 18.47%
#################################################
```

## Run a BIDL Network

You can start a minimal BIDL network up with the following command:

```bash
bash ./run-bidl-local.sh test
```

Upon successful completion, the logs should report the following in your terminal window:

```bash
#################################################
Submission rate 50 throughput 50 kTxns/s
Submission rate 50 consensus latency 3 ms
Submission rate 50 execution latency 7 ms
Submission rate 50 commit latency 1 ms
#################################################
```
