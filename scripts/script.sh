#!/bin/bash

echo
echo " ____    _____      _      ____    _____ "
echo "/ ___|  |_   _|    / \    |  _ \  |_   _|"
echo "\___ \    | |     / _ \   | |_) |   | |  "
echo " ___) |   | |    / ___ \  |  _ <    | |  "
echo "|____/    |_|   /_/   \_\ |_| \_\   |_|  "
echo
echo "Build your first network (BYFN) end-to-end test"
echo
CHANNEL_NAME="$1"
DELAY="$2"
LANGUAGE="$3"
TIMEOUT="$4"
VERBOSE="$5"
: ${CHANNEL_NAME:="mychannel"}
: ${DELAY:="5"}
: ${LANGUAGE:="golang"}
: ${TIMEOUT:="10"}
: ${VERBOSE:="false"}
LANGUAGE=`echo "$LANGUAGE" | tr [:upper:] [:lower:]`
COUNTER=1
MAX_RETRY=10
all_peers=6

CC_SRC_PATH="github.com/chaincode/smallbank"

echo "Channel name : "$CHANNEL_NAME

# import utils
. scripts/utils.sh
let all_peers=all_peers-1

createChannel() {
	setGlobals peer0 1
	if [ -z "$CORE_PEER_TLS_ENABLED" -o "$CORE_PEER_TLS_ENABLED" = "false" ]; then
                set -x
		peer channel create -o fabric_orderer:7050 -c $CHANNEL_NAME -f ./channel-artifacts/channel.tx >&log.txt 
		res=$?
                set +x
	else
				set -x
		peer channel create -o fabric_orderer:7050 -c $CHANNEL_NAME -f ./channel-artifacts/channel.tx --tls $CORE_PEER_TLS_ENABLED --cafile $ORDERER_CA >&log.txt
		res=$?
				set +x
	fi
	cat log.txt
	verifyResult $res "Channel creation failed"
	echo "===================== Channel '$CHANNEL_NAME' created ===================== "
	echo
}

joinChannel () {
	for org in 1; do
	    for peer in `seq 0 $all_peers`; do
			joinChannelWithRetry peer$peer $org
			echo "===================== peer${peer}.org${org} joined channel '$CHANNEL_NAME' ===================== "
			# sleep $DELAY
		echo
	    done
	done
}
### Create channel
echo "Creating channel..."
createChannel
sleep 2
#
### Join all the peers to the channel
#echo "Having all peers join the channel..."
joinChannel
sleep 2
updateAnchorPeers peer0 1
sleep 2
#
### Install chaincode on peer0.org1 and peer0.org2
#echo "Installing chaincode on peer0.org1..."
for peer in `seq 0 $all_peers`; do
	echo "install chaincode on peer$peer"
	installChaincode peer$peer 1
done
# ## Instantiate chaincode on peer0.org2
# #echo "Instantiating chaincode on peer0.org2..."
sleep 5
instantiateChaincode peer0 1
# 
sleep 2
createAccount 0 1 
sleep 2
for i in `seq 0 $all_peers`; do
	# setup chaincode container 
	chaincodeQuery $i 1 a
done
echo
echo "========= All GOOD, BYFN execution completed =========== "
echo

echo
echo " _____   _   _   ____   "
echo "| ____| | \ | | |  _ \  "
echo "|  _|   |  \| | | | | | "
echo "| |___  | |\  | | |_| | "
echo "|_____| |_| \_| |____/  "
echo

exit 0
