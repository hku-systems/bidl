# Usage: gen_orderer.sh $1:ORDERER $2:ORDERER_INDEX

file=orderer.yaml
rm -f $file
echo "---

General:
    LedgerType: file
    ListenAddress: 0.0.0.0
    ListenPort: 7050
    TLS:
        Enabled: true
        PrivateKey: crypto-config/ordererOrganizations/example.com/orderers/orderer$2.example.com/tls/server.key
        Certificate: crypto-config/ordererOrganizations/example.com/orderers/orderer$2.example.com/tls/server.crt
        RootCAs:
          - crypto-config/ordererOrganizations/example.com/orderers/orderer$2.example.com/tls/ca.crt
        ClientAuthRequired: false
        ClientRootCAs:
    Keepalive:
        ServerMinInterval: 60s
        ServerInterval: 7200s
        ServerTimeout: 20s
    Cluster:
        SendBufferSize: 10
        ClientCertificate: crypto-config/ordererOrganizations/example.com/orderers/orderer$2.example.com/tls/server.crt
        ClientPrivateKey: crypto-config/ordererOrganizations/example.com/orderers/orderer$2.example.com/tls/server.key
        DialTimeout: 5s
        RPCTimeout: 7s
        RootCAs:
          - crypto-config/ordererOrganizations/example.com/orderers/orderer$2.example.com/tls/ca.crt
        ReplicationBufferSize: 20971520 # 20MB
        ReplicationPullTimeout: 5s
        ReplicationRetryTimeout: 5s
        ListenPort:
        ListenAddress:
        ServerCertificate:
        ServerPrivateKey:
    GenesisMethod: file
    GenesisProfile: OneOrgOrdererGenesis
    GenesisFile: config/genesis.block
    LocalMSPDir: crypto-config/ordererOrganizations/example.com/orderers/orderer$2.example.com/msp" > $file

    if [ "$mode" = "streamchain" ] || [ "$mode" = "streamchain_ff" ] ; then
    	echo "    LocalMSPType: noop" >> $file
    fi

    echo "    LocalMSPID: OrdererMSP
    Profile:
        Enabled: false
        Address: 0.0.0.0:6060
    BCCSP:
        Default: SW
        SW:
            Hash: SHA2
            Security: 256
            FileKeyStore:
                KeyStore:
    Authentication:
        TimeWindow: 15m

FileLedger:
    Location: $orderer_data
    Prefix: hyperledger-fabric-ordererledger

RAMLedger:
    HistorySize: 1000

Kafka:
    Retry:
        ShortInterval: 5s
        ShortTotal: 10m
        LongInterval: 5m
        LongTotal: 12h
        NetworkTimeouts:
            DialTimeout: 10s
            ReadTimeout: 10s
            WriteTimeout: 10s
        Metadata:
            RetryBackoff: 250ms
            RetryMax: 3
        Producer:
            RetryBackoff: 100ms
            RetryMax: 3
        Consumer:
            RetryBackoff: 2s
    Topic:
        ReplicationFactor: 3
    Verbose: false
    TLS:
      Enabled: false
      PrivateKey:
      Certificate:
      RootCAs:
    SASLPlain:
      Enabled: false
      User:
      Password:
    Version:

Debug:
    BroadcastTraceDir:
    DeliverTraceDir:

Operations:
    ListenAddress: 127.0.0.1:8443
    TLS:
        Enabled: false
        Certificate:
        PrivateKey:
        ClientAuthRequired: false
        RootCAs: []

Metrics:
    Provider: disabled
    Statsd:
      Network: udp
      Address: 127.0.0.1:8125
      WriteInterval: 30s
      Prefix:

Consensus:
    WALDir: $orderer_data/etcdraft/wal
    SnapDir: $orderer_data/etcdraft/snapshot
" >> $file

scp -q $file $user@$1:$bm_path
