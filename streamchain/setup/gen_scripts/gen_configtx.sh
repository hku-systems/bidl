# Usage: gen_configtx.sh

file=configtx.yaml
rm -f $file
echo "Organizations:
    - &OrdererOrg
        Name: OrdererOrg
        ID: OrdererMSP" >> $file
if [ "$mode" = "streamchain" ] || [ "$mode" = "streamchain_ff" ] ; then
echo "        MSPType: noop" >> $file
fi
echo "        MSPDir: crypto-config/ordererOrganizations/example.com/msp
        Policies:
            Readers:
                Type: Signature
                Rule: \"OR('OrdererMSP.member')\"
            Writers:
                Type: Signature
                Rule: \"OR('OrdererMSP.member')\"
            Admins:
                Type: Signature
                Rule: \"OR('OrdererMSP.admin')\"
    - &Org1
        Name: Org1MSP
        ID: Org1MSP
        MSPDir: crypto-config/peerOrganizations/org1.example.com/msp
        Policies: &Org1Policies
            Readers:
                Type: Signature
                Rule: \"OR('Org1MSP.member')\"
            Writers:
                Type: Signature
                Rule: \"OR('Org1MSP.member')\"
            Admins:
                Type: Signature
                Rule: \"OR('Org1MSP.admin')\"
        AnchorPeers:
            - Host: $event
              Port: 7051

Capabilities:
    Channel: &ChannelCapabilities
        V1_3: true
    Orderer: &OrdererCapabilities
        V1_1: true
    Application: &ApplicationCapabilities
        V1_3: true
        V1_2: false
        V1_1: false

Application: &ApplicationDefaults
    Organizations:
    Policies: &ApplicationDefaultPolicies
        Readers:
            Type: ImplicitMeta
            Rule: \"ANY Readers\"
        Writers:
            Type: ImplicitMeta
            Rule: \"ANY Writers\"
        Admins:
            Type: ImplicitMeta
            Rule: \"MAJORITY Admins\"
#Capabilities:
#    <<: *ApplicationCapabilities

Orderer: &OrdererDefaults
    OrdererType: solo
    Addresses:" >> $file

for i in $orderers ; do
    echo "        - $i:7050" >> $file
done

echo "    BatchTimeout: 2s
    BatchSize:
        MaxMessageCount: $txs
        AbsoluteMaxBytes: 99 MB
        PreferredMaxBytes: $((10*$txs)) KB
    MaxChannels: 0
    Kafka:
        Brokers:
            - 127.0.0.1:9092
    EtcdRaft:
        Consenters:" >> $file

i=0
for o in $orderers ; do
echo "            - Host: $o
              Port: 7050
              ClientTLSCert: crypto-config/ordererOrganizations/example.com/orderers/orderer$i.example.com/tls/server.crt
              ServerTLSCert: crypto-config/ordererOrganizations/example.com/orderers/orderer$i.example.com/tls/server.crt" >> $file
i=$(($i+1))
done
echo "
    Organizations:
    Policies:
        Readers:
            Type: ImplicitMeta
            Rule: \"ANY Readers\"
        Writers:
            Type: ImplicitMeta
            Rule: \"ANY Writers\"
        Admins:
            Type: ImplicitMeta
            Rule: \"MAJORITY Admins\"
        BlockValidation:
            Type: ImplicitMeta
            Rule: \"ANY Writers\"
    Capabilities:
        <<: *OrdererCapabilities

Channel: &ChannelDefaults
    Policies:
        Readers:
            Type: ImplicitMeta
            Rule: \"ANY Readers\"
        Writers:
            Type: ImplicitMeta
            Rule: \"ANY Writers\"
        Admins:
            Type: ImplicitMeta
            Rule: \"MAJORITY Admins\"
    Capabilities:
        <<: *ChannelCapabilities

Profiles:
    OneOrgOrdererGenesis:
        <<: *ChannelDefaults
        Orderer:
            <<: *OrdererDefaults
            Organizations:
                - *OrdererOrg
        Capabilities:
            <<: *OrdererCapabilities
        Consortiums:
            SampleConsortium:
                Organizations:
                    - *Org1
    OneOrgChannel:
        Consortium: SampleConsortium
        <<: *ChannelDefaults
        Application:
            <<: *ApplicationDefaults
            Organizations:
                - *Org1
    MultiNodeEtcdRaft:
        <<: *ChannelDefaults
        Capabilities:
            <<: *ChannelCapabilities
        Orderer:
            <<: *OrdererDefaults
            OrdererType: etcdraft
            Organizations:
                - *OrdererOrg
            Capabilities:
                <<: *OrdererCapabilities
        Application:
            <<: *ApplicationDefaults
            Organizations:
             - <<: *Org1
        Consortiums:
            SampleConsortium:
                Organizations:
                - *Org1" >> $file
