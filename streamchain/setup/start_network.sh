#. config.sh
export FABRIC_LOGGING_SPEC=CRITICAL

aorderers=($orderers)

i=0
for orderer in $orderers ; do
    mkdir -p logs/$orderer
    echo "Starting orderer node $i.."
    ssh -f $user@$orderer "cd $bm_path ; FABRIC_LOGGING_SPEC=CRITICAL ./orderer > orderer.log"
    i=$((i+1))
done

i=0
for peer in $peers ; do
    echo "Starting peer node $i.."
    ssh -f $user@$peer "cd $bm_path ; FABRIC_LOGGING_SPEC=CRITICAL STREAMCHAIN_WRITEBATCH=$STREAMCHAIN_WRITEBATCH STREAMCHAIN_ADDBLOCK=$STREAMCHAIN_ADDBLOCK STREAMCHAIN_PIPELINE=$STREAMCHAIN_PIPELINE STREAMCHAIN_VSCC=$STREAMCHAIN_VSCC STREAMCHAIN_SYNCDB=$STREAMCHAIN_SYNCDB ./peer node start > peer.log"
    i=$((i+1))
done

sleep 2

echo 'Create channel..'
$run_event "FABRIC_LOGGING_SPEC=CRITICAL ./peer channel create -o ${aorderers[0]}:7050 -c mychannel -f ./config/channel.tx --tls --cafile crypto-config/ordererOrganizations/example.com/orderers/orderer0.example.com/msp/tlscacerts/tlsca.example.com-cert.pem"

echo 'Join peers to channel..'
for peer in $peers ; do
    ssh $user@$peer "cd $bm_path ; if [ ! -f ./mychannel.block ] ; then FABRIC_LOGGING_SPEC=CRITICAL ./peer channel fetch 0 mychannel.block -c mychannel -o ${aorderers[0]}:7050 --tls --cafile crypto-config/ordererOrganizations/example.com/orderers/orderer0.example.com/msp/tlscacerts/tlsca.example.com-cert.pem ; fi"
    ssh $user@$peer "cd $bm_path ; FABRIC_LOGGING_SPEC=CRITICAL ./peer channel join -b mychannel.block"
    ssh $user@$peer "cd $bm_path ; FABRIC_LOGGING_SPEC=CRITICAL GOPATH=$GOPATH PATH=$goexec:\$PATH ./peer chaincode install -n YCSB -v 1.0 -p github.com/chaincode/YCSB/"
done

echo 'Instantiate chaincode in channel..'
$run_event "FABRIC_LOGGING_SPEC=CRITICAL ./peer chaincode instantiate -o ${aorderers[0]}:7050 -C mychannel -n YCSB -v 1.0 -c '{\"Args\":[\"init\"]}' -P \"OR ('Org1MSP.member')\" --tls --cafile crypto-config/ordererOrganizations/example.com/orderers/orderer0.example.com/msp/tlscacerts/tlsca.example.com-cert.pem"

sleep 4
