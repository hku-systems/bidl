#!/usr/bin/bash
export PATH=$PATH:./bin
. config.sh

SYS=$1
CHANNEL=mychannel

if [[ -d ./organizations ]]; then rm -r ./organizations; fi
if [[ -d ./channel-artifacts ]]; then rm -r ./channel-artifacts; fi

# mkdir peerOrganizations
mkdir channel-artifacts
cryptogen generate --config=./crypto-config.yaml --output="organizations"
configtxgen -configPath ./ -outputBlock ./channel-artifacts/genesis.block -profile OneOrgOrdererGenesis -channelID ${CHANNEL}-system-channel
configtxgen -configPath ./ -outputCreateChannelTx ./channel-artifacts/channel.tx -profile OneOrgChannel -channelID ${CHANNEL}
configtxgen -configPath ./ -outputAnchorPeersUpdate ./channel-artifacts/anchor_peer.tx -profile OneOrgChannel -asOrg Org1MSP -channelID ${CHANNEL}

cp organizations/peerOrganizations/org1.example.com/users/User1@org1.example.com/msp/keystore/*sk organizations/peerOrganizations/org1.example.com/users/User1@org1.example.com/msp/keystore/priv_sk

tar -czvf data.tar.gz organizations channel-artifacts scripts chaincode tape.yaml > /dev/null

for host in ${hosts[@]}; do 
    ssh $USER@$host "rm -rf ${SYS}_exp; mkdir ${SYS}_exp"
    scp data.tar.gz $USER@$host:~/${SYS}_exp/ 
    ssh $USER@$host "cd ${SYS}_exp; tar -xzvf data.tar.gz > /dev/null; rm data.tar.gz"
done
rm data.tar.gz


echo "generate docker-compose.yaml"
if [[ ! -f config-${SYS}.yaml ]]; then 
    echo "config-${SYS}.yaml not exist!"
    exit 1
fi
python3 generate.py $SYS


