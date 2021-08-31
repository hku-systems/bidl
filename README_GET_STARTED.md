# Get Started Tutorial: Run a Hyperledger Fabric network  

You can bring the network up with the following command: 
```bash 
bash example.sh 
```
The above command will: 
1. start a Fabric network with one orderer and five peers
2. create a channel
3. deploy the smallbank smart contract: `chaincode/smallbank/smallbank.go`
4. create 5000 accounts
5. send 5000 payment transactions

Upon successful completion, the logs should report the following in your terminal
window: 

```bash 
#################################################
create 5000 acocunts
tx: 5000, duration: 10.37808749s, tps: 481.784337
start 5000 payment transactions
tx: 5000, duration: 10.377495212s, tps: 481.811834
valid: 4794/5000 95.88% invalid: 206/5000 4.12%
#################################################
```

