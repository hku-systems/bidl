#!/bin/bash
# rm -rf .build/docker/bin/peer
# rm -rf .build/docker/bin/orderer
rm -rf .build/image/orderer/.dummy-amd64-1.3.0-snapshot-dd0f323
rm -rf .build/image/peer/.dummy-amd64-1.3.0-snapshot-dd0f323

# make native
docker image rm -f hyperledger/fabric-orderer:latest
docker image rm -f hyperledger/fabric-peer:latest
docker image rm -f hyperledger/fabric-orderer:amd64-latest
docker image rm -f hyperledger/fabric-peer:amd64-latest
docker image rm -f hyperledger/fabric-orderer:amd64-1.3.0-snapshot-dd0f323
docker image rm -f hyperledger/fabric-peer:amd64-1.3.0-snapshot-dd0f323

make docker