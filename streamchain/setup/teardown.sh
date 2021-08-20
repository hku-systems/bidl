# Usage: teardown.sh

. config.sh

echo 'Killing peer processes..'
for peer in $peers ; do
    echo $peer
    ssh $user@$peer 'killall -q peer'
done

echo 'Killing orderers..'
for orderer in $orderers ; do
    ssh $user@$orderer 'killall -q orderer'
done

echo 'Deleting data on peers..'
for peer in $peers ; do
    ssh $user@$peer "cd $bm_path; rm -rf $peer_data/chaincodes $peer_data/ledgersData $peer_data/transientStore mychannel.block $statedb_data/stateLeveldb"
done

echo 'Deleting data on orderers..'
for orderer in $orderers ; do
    ssh $user@$orderer "cd $bm_path; rm -rf $orderer_data/chains $orderer_data/index $orderer_data/etcdraft"
done

echo 'Deleting generated files..'
rm -rf configtx.yaml crypto-config.yaml core.yaml orderer.yaml
