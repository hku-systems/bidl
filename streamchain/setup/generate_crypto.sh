#!/bin/sh

export FABRIC_CFG_PATH=${PWD}
CHANNEL_NAME=mychannel

echo 'Cleanup old crypto material..'
rm -rf crypto-config

for i in $orderers $peers ; do
	ssh $user@$i "rm -rf $bm_path/crypto-config/*"
done

echo 'Generate new crypto material..'
./bin/cryptogen generate --config=./crypto-config.yaml
if [ "$?" -ne 0 ]; then
  echo "Failed to generate crypto material..."
  exit 1
fi

for i in $orderers $peers ; do
    scp -rq crypto-config $USER@$i:$bm_path
done
