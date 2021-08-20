#!/bin/sh

export FABRIC_CFG_PATH=${PWD}
CHANNEL_NAME=mychannel

# cleanup
for i in $peers $orderers ; do
    ssh $USER@$i rm -rf $bm_path/config/*
done

mkdir -p config

# generate genesis block for orderer
if [ "$raft" = true ] ; then
	./bin/configtxgen -profile MultiNodeEtcdRaft -outputBlock ./config/genesis.block
	
else
	./bin/configtxgen -profile OneOrgOrdererGenesis -outputBlock ./config/genesis.block
fi

if [ "$?" -ne 0 ]; then
  echo "Failed to generate orderer genesis block..."
  exit 1
fi

# generate channel configuration transaction
./bin/configtxgen -profile OneOrgChannel -outputCreateChannelTx ./config/channel.tx -channelID $CHANNEL_NAME
if [ "$?" -ne 0 ]; then
  echo "Failed to generate channel configuration transaction..."
  exit 1
fi

# generate anchor peer transaction
./bin/configtxgen -profile OneOrgChannel -outputAnchorPeersUpdate ./config/Org1MSPanchors.tx -channelID $CHANNEL_NAME -asOrg Org1MSP
if [ "$?" -ne 0 ]; then
  echo "Failed to generate anchor peer update for Org1MSP..."
  exit 1
fi

for i in $peers $orderers ; do
    scp -rq config $USER@$i:$bm_path
done
