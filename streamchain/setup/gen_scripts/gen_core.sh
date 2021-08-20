# Usage: gen_core.sh $1:PEER $2:IS_ORG_LEADER $3:INDEX

file=core.yaml
rm -f $file
echo "logging:
    cauthdsl:   error
    gossip:     error
    grpc:       error
    ledger:     error
    msp:        error
    policies:   error
    peer:
        gossip: error
    format: '%{color}%{time:2006-01-02 15:04:05.000 MST} [%{module}] %{shortfunc} -> %{level:.4s} %{id:03x}%{color:reset} %{message}'
peer:
    id: peer$3
    networkId: dev
    listenAddress: 0.0.0.0:7051
    address: $1:7051
    addressAutoDetect: false
    gomaxprocs: -1
    keepalive:
        minInterval: 60s
        client:
            interval: 60s
            timeout: 20s
        deliveryClient:
            interval: 60s
            timeout: 20s
    gossip:
        bootstrap: $1:7051
        useLeaderElection: false
        orgLeader: true
        endpoint:
        maxBlockCountToStore: 100
        maxPropagationBurstLatency: 10ms
        maxPropagationBurstSize: 10
        propagateIterations: 1
        propagatePeerNum: 3
        pullInterval: 4s
        pullPeerNum: 3
        requestStateInfoInterval: 4s
        publishStateInfoInterval: 4s
        stateInfoRetentionInterval:
        publishCertPeriod: 10s
        skipBlockVerification: false
        dialTimeout: 3s
        connTimeout: 2s
        recvBuffSize: 20
        sendBuffSize: 200
        digestWaitTime: 1s
        requestWaitTime: 1500ms
        responseWaitTime: 2s
        aliveTimeInterval: 5s
        aliveExpirationTimeout: 25s
        reconnectInterval: 25s
        externalEndpoint: $2
        election:
            startupGracePeriod: 15s
            membershipSampleInterval: 1s
            leaderAliveThreshold: 10s
            leaderElectionDuration: 5s
        pvtData:
            pullRetryThreshold: 60s
            transientstoreMaxBlockRetention: 1000
            pushAckTimeout: 3s
            btlPullMargin: 10
    events:
        address: 0.0.0.0:7053
        buffersize: 100
        timeout: 10ms
        timewindow: 15m
        keepalive:
            minInterval: 60s
        sendTimeout: 60s
    tls:
        enabled: true
        clientAuthRequired: false
        cert:
            file: crypto-config/peerOrganizations/org1.example.com/peers/peer$3.org1.example.com/tls/server.crt
        key:
            file: crypto-config/peerOrganizations/org1.example.com/peers/peer$3.org1.example.com/tls/server.key
        rootcert:
            file: crypto-config/peerOrganizations/org1.example.com/peers/peer$3.org1.example.com/tls/ca.crt
        clientRootCAs:
            files:
              - crypto-config/peerOrganizations/org1.example.com/peers/peer$3.org1.example.com/tls/ca.crt
        clientKey:
            file:
        clientCert:
            file:
    authentication:
        timewindow: 15m
    fileSystemPath: $peer_data
    stateDBPath: $statedb_data
    BCCSP:
        Default: SW
        SW:
            Hash: SHA2
            Security: 256
            FileKeyStore:
                KeyStore:
        PKCS11:
            Library:
            Label:
            Pin:
            Hash:
            Security:
            FileKeyStore:
                KeyStore:
    mspConfigPath: crypto-config/peerOrganizations/org1.example.com/users/Admin@org1.example.com/msp
    localMspId: Org1MSP
    client:
        connTimeout: 3s
    deliveryclient:
        reconnectTotalTimeThreshold: 3600s
        connTimeout: 3s
        reConnectBackoffThreshold: 3600s
    localMspType: bccsp
    profile:
        enabled:     false
        listenAddress: 0.0.0.0:6060
    adminService:
        #listenAddress: 0.0.0.0:7055
    handlers:
        authFilters:
          -
            name: DefaultAuth
          -
            name: ExpirationCheck
        decorators:
          -
            name: DefaultDecorator
        endorsers:
          escc:
            name: DefaultEndorsement
            library:
        validators:
          vscc:
            name: DefaultValidation
            library:
    validatorPoolSize:
    discovery:
        enabled: true
        authCacheEnabled: true
        authCacheMaxSize: 1000
        authCachePurgeRetentionRatio: 0.75
        orgMembersAllowedAccess: false
vm:
    endpoint: unix:///var/run/docker.sock
    docker:
        tls:
            enabled: true
            ca:
                file: docker/ca.crt
            cert:
                file: docker/tls.crt
            key:
                file: docker/tls.key
        attachStdout: false
        hostConfig:
            NetworkMode: host
            Dns:
                LogConfig:
                Type: json-file
                Config:
                    max-size: \"50m\"
                    max-file: \"5\"
            Memory: 2147483648
chaincode:
    id:
        path:
        name:
    builder: \$(DOCKER_NS)/fabric-ccenv:latest
    pull: false
    golang:
        runtime: \$(BASE_DOCKER_NS)/fabric-baseos:\$(ARCH)-\$(BASE_VERSION)
        dynamicLink: false
    car:
        runtime: \$(BASE_DOCKER_NS)/fabric-baseos:\$(ARCH)-\$(BASE_VERSION)

    java:
        runtime: \$(DOCKER_NS)/fabric-javaenv:\$(ARCH)-\$(PROJECT_VERSION)

    node:
        runtime: \$(BASE_DOCKER_NS)/fabric-baseimage:\$(ARCH)-\$(BASE_VERSION)
    startuptimeout: 300s
    executetimeout: 30s
    mode: net
    keepalive: 0
    system:
        cscc: enable
        lscc: enable
        escc: enable
        vscc: enable
        qscc: enable
    systemPlugins:
    logging:
      level:  error
      shim:   error
      format: '%{color}%{time:2006-01-02 15:04:05.000 MST} [%{module}] %{shortfunc} -> %{level:.4s} %{id:03x}%{color:reset} %{message}'
ledger:
  blockchain:
  state:
    stateDatabase: goleveldb
    couchDBConfig:
       couchDBAddress: 127.0.0.1:5984
       username:
       password:
       maxRetries: 3
       maxRetriesOnStartup: 10
       requestTimeout: 35s
       queryLimit: 10000
       maxBatchUpdateSize: 1000
       warmIndexesAfterNBlocks: 1
  history:
    enableHistoryDatabase: false
metrics:
        enabled: false
        reporter: statsd
        interval: 1s
        statsdReporter:
              address: 0.0.0.0:8125
              flushInterval: 2s
              flushBytes: 1432
        promReporter:
              listenAddress: 0.0.0.0:8080" >> $file

scp -q $file $USER@$1:$bm_path
