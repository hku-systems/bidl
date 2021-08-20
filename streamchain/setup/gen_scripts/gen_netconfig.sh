# Usage: gen_netconfig.sh

file=netconfig.yaml
rm -f $file
echo "name: \"basic-network\"
x-type: \"hlfv1\"
description: \"The basic network\"
version: \"1.0\"
client:
   organization: Org1
   credentialStore:
      path: \"crypto/config\"
      cryptoStore:
         path: \"crypto/config\"
channels:
  mychannel:
    orderers:
      - orderer.example.com
    peers:" >> $file
    
i=0
for peer in $peers ; do
  echo "      peer$i.org1.example.com:
        endorsingPeer: true
        chaincodeQuery: true
        ledgerQuery: true
        eventSource: true" >> $file
	i=$(($i+1))
done

echo "organizations:
  Org1:
    mspid: Org1MSP
    peers:" >> $file

i=0
for peer in $peers ; do
	echo "        - peer$i.org1.example.com" >> $file
	i=$(($i+1))
done

echo "    certificateAuthorities:
      - ca-org1
orderers:
  orderer.example.com:
    url: grpcs://$orderer:7050
    grpcOptions:
      ssl-target-name-override: orderer.example.com
peers:" >> $file

i=0
for peer in $peers ; do
	echo "  peer$i.org1.example.com:
    url: grpcs://$peer:7051
    grpcOptions:
      ssl-target-name-override: peer$i.org1.example.com
      request-timeout: 120001
    tlsCaCerts:
      path: crypto-config/ordererOrganizations/example.com/orderers/orderer.example.com/msp/tlscacerts/tlsca.example.com-cert.pem" >> $file
	i=$(($i+1))
done

echo "certificateAuthorities:
  ca-org1:
    url: https://11.1.212.200:7054
    httpOptions:
      verify: false
    registrar:
      - enrollId: admin
        enrollSecret: adminpw
    caName: ca-org1" >> $file

scp -q $file $event_dir
