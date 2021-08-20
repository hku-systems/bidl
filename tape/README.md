# tape

# Build
- local: `go build ./cmd/tape`       
- docker: `docker build -f Dockerfile -t tape .`

# Run
1. Link to crypto materials: `ln -sf $YOUR_PROJECT/organizations`
2. End-to-End Run     
    ```bash
    # if(ACCOUNTS not exists):
    #   create ACCOUNTS  
    # else:
    #   send transactions (i.e. transfer money from A to B) 
    # end

    # typically 
    rm ACCOUNTS # clean old accounts
    ./tape --e2e --hrate 0.1 --crate 0.1 --txtype create_random --config config.yaml -n 1000  # randomly create 1000 accounts according config.yaml
    ./tape --e2e --hrate 0.1 --crate 0.1 --txtype create --config config.yaml -n 1000  # create 1000 accounts according config.yaml
    ./tape --e2e --hrate 0.1 --crate 0.1 --txtype transfer --config config.yaml -n 10000  # send 10000 transactions using ACCOUNTS
    ```
3. Breakdown      
    ```bash
    # if(EDNORSEMENT not exists):
    #   start phase1 to create ENDORSEMENT: send proposals to endorsers
    # else:
    #   start phase2 to broadcast transactions to orderer (read from ENDORSEMENT)
    # end

    # typically 
    rm ENDORSEMENT # clean old endorsements
    ./tape --no-e2e --hrate 0.1 --crate 0.1 --txtype create --config config.yaml -n 10000  # create 10000 endorsements
    ./tape --no-e2e --hrate 0.1 --crate 0.1 --txtype create --config config.yaml -n 10000  # broadcast 10000 transactions 
    ```
4. Nondeterminism 
    ```bash 
    rm ACCOUNTS
    ./tape --no-e2e --txtype create_random --ndrate 0.5 --config.yaml -n 1000

    ```
# Result
Save output to file for analysis: `./tape -c config.yaml -n 10000  > log.transactions `

## latency breakdown
```
cat log.transactions | python3 scripts/latency.py > latency.log  
```

**Output format:** txid [#1, #2, #2]
1. endorseement: clients sends proposal => client receives enough endorsement
2. local_process: clients generate signed transaction based on endorsements
3. consensus & commit: clients send signed transaction => clients receive response (including consensus, validation, and commit)


## conflict rate
```bash
bash scripts/conflict.sh
```

# TODO
1. zipfan distribution workload
2. add prometheus 
